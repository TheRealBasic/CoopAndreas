#include "stdafx.h"
#include "CNetworkVehicle.h"
#include <unordered_map>

std::vector<CNetworkVehicle*> CNetworkVehicleManager::m_pVehicles;
CNetworkVehicle* CNetworkVehicleManager::m_apTempVehicles[255];
CNetworkVehicleManager::StreamConfig CNetworkVehicleManager::m_streamConfig{};
CNetworkVehicleManager::Telemetry CNetworkVehicleManager::m_telemetry{};

static std::unordered_map<int, size_t> s_vehicleModelRefs;
static uint32_t s_lastVehicleStreamTick = 0;
static uint32_t s_streamChurnWindowStart = 0;
static size_t s_streamChurnCounter = 0;

CNetworkVehicle* CNetworkVehicleManager::GetVehicle(int vehicleid)
{
	for (int i = 0; i != m_pVehicles.size(); i++)
	{
		if (m_pVehicles[i]->m_nVehicleId == vehicleid)
		{
			return m_pVehicles[i];
		}
	}
	return nullptr;
}

CNetworkVehicle* CNetworkVehicleManager::GetVehicle(CVehicle* vehicle)
{
	for (int i = 0; i != m_pVehicles.size(); i++)
	{
		if (m_pVehicles[i]->m_pVehicle == vehicle)
		{
			return m_pVehicles[i];
		}
	}
	
	return nullptr;
}

CNetworkVehicle* CNetworkVehicleManager::GetVehicle(CEntity* vehicle)
{
	for (int i = 0; i != m_pVehicles.size(); i++)
	{
		if (m_pVehicles[i]->m_pVehicle == vehicle)
		{
			return m_pVehicles[i];
		}
	}

	return nullptr;
}

void CNetworkVehicleManager::Add(CNetworkVehicle* vehicle)
{
	CNetworkVehicleManager::m_pVehicles.push_back(vehicle);
	if (vehicle)
	{
		AddModelRef(vehicle->m_nModelId);
	}
}

void CNetworkVehicleManager::Remove(CNetworkVehicle* vehicle)
{
	auto it = std::find(m_pVehicles.begin(), m_pVehicles.end(), vehicle);
	if (it != m_pVehicles.end())
	{
		if (vehicle)
		{
			ReleaseModelRef(vehicle->m_nModelId);
		}
		m_pVehicles.erase(it);
	}
}

void CNetworkVehicleManager::UpdateDriver(CVehicle* vehicle)
{
	if (auto networkVehicle = CNetworkVehicleManager::GetVehicle(vehicle))
	{
		CPackets::VehicleDriverUpdate* packet = CPacketHandler::VehicleDriverUpdate__Collect(networkVehicle);
		CNetwork::SendPacket(CPacketsID::VEHICLE_DRIVER_UPDATE, packet, sizeof *packet);
		delete packet;
	}
}

void CNetworkVehicleManager::UpdateIdle()
{
	CNetworkVehicleManager::RemoveHostedUnused();
	TickStreaming(GetTickCount());

	CMassPacketBuilder builder;

	for (int i = 0; i != m_pVehicles.size(); i++)
	{
		if (m_pVehicles[i]->m_pVehicle == nullptr)
			continue;

		if (m_pVehicles[i]->m_bSyncing && !m_pVehicles[i]->HasDriver())
		{
			CPackets::VehicleIdleUpdate* packet = CPacketHandler::VehicleIdleUpdate__Collect(m_pVehicles[i]);
			builder.AddPacket(CPacketsID::VEHICLE_IDLE_UPDATE, packet, sizeof *packet);
			delete packet;
		}
	}

	builder.Send();
}

void CNetworkVehicleManager::TickStreaming(uint32_t tickCount)
{
	if (tickCount < s_lastVehicleStreamTick + m_streamConfig.tickIntervalMs)
		return;

	s_lastVehicleStreamTick = tickCount;
	if (!FindPlayerPed(0) || !FindPlayerPed(0)->m_matrix)
		return;

	CVector localPos = FindPlayerPed(0)->m_matrix->pos;
	size_t streamedNow = 0;

	for (auto* vehicle : m_pVehicles)
	{
		if (!vehicle)
			continue;

		bool shouldStayStreamed = vehicle->m_bMissionCritical;
		CVector delta = vehicle->m_cachedState.position - localPos;
		float distance = delta.Magnitude();
		if (!shouldStayStreamed)
		{
			if (vehicle->m_eStreamState == CNetworkVehicle::eStreamState::Streamed)
				shouldStayStreamed = distance <= m_streamConfig.streamOutRadius;
			else
				shouldStayStreamed = distance <= m_streamConfig.streamInRadius;
		}

		if (shouldStayStreamed && streamedNow < m_streamConfig.maxStreamed)
		{
			if (!vehicle->m_pVehicle)
			{
				if (!vehicle->StreamIn())
					m_telemetry.failedSpawns++;
				s_streamChurnCounter++;
			}
			streamedNow++;
		}
		else if (vehicle->m_pVehicle)
		{
			vehicle->StreamOut();
			s_streamChurnCounter++;
		}
	}

	if (s_streamChurnWindowStart == 0)
		s_streamChurnWindowStart = tickCount;
	if (tickCount - s_streamChurnWindowStart >= 60000)
	{
		m_telemetry.streamChurnPerMinute = s_streamChurnCounter;
		s_streamChurnCounter = 0;
		s_streamChurnWindowStart = tickCount;
	}

	m_telemetry.totalNetworkEntities = m_pVehicles.size();
	m_telemetry.streamedEntities = streamedNow;
	m_telemetry.pendingModelLoads = 0;
	for (const auto& [modelId, refs] : s_vehicleModelRefs)
	{
		if (refs > 0 && CStreaming::ms_aInfoForModel[modelId].m_nLoadState != LOADSTATE_LOADED)
			m_telemetry.pendingModelLoads++;
	}
}

void CNetworkVehicleManager::CacheNetworkState(CNetworkVehicle* vehicle, const CPackets::VehicleIdleUpdate& packet)
{
	if (!vehicle)
		return;

	vehicle->m_cachedState.position = packet.pos;
	vehicle->m_cachedState.roll = packet.roll;
	vehicle->m_cachedState.up = packet.rot;
	vehicle->m_cachedState.velocity = packet.velocity;
	vehicle->m_cachedState.turnSpeed = packet.turnSpeed;
	vehicle->m_cachedState.color1 = packet.color1;
	vehicle->m_cachedState.color2 = packet.color2;
	vehicle->m_cachedState.health = packet.health;
	vehicle->m_cachedState.locked = (eDoorLock)packet.locked;
}

void CNetworkVehicleManager::CacheNetworkState(CNetworkVehicle* vehicle, const CPackets::VehicleDriverUpdate& packet)
{
	if (!vehicle)
		return;

	vehicle->m_cachedState.position = packet.pos;
	vehicle->m_cachedState.roll = packet.roll;
	vehicle->m_cachedState.up = packet.rot;
	vehicle->m_cachedState.velocity = packet.velocity;
	vehicle->m_cachedState.color1 = packet.color1;
	vehicle->m_cachedState.color2 = packet.color2;
	vehicle->m_cachedState.health = packet.health;
	vehicle->m_cachedState.paintjob = packet.paintjob;
	vehicle->m_cachedState.locked = (eDoorLock)packet.locked;
	vehicle->m_cachedState.driverPlayerId = packet.playerid;
}

void CNetworkVehicleManager::AddModelRef(int modelId)
{
	if (modelId < 0)
		return;
	s_vehicleModelRefs[modelId]++;
}

void CNetworkVehicleManager::ReleaseModelRef(int modelId)
{
	auto it = s_vehicleModelRefs.find(modelId);
	if (it == s_vehicleModelRefs.end())
		return;

	if (it->second > 0)
		it->second--;

	if (it->second == 0)
	{
		CStreaming::SetModelIsDeletable(modelId);
		CStreaming::SetModelTxdIsDeletable(modelId);
		s_vehicleModelRefs.erase(it);
	}
}

const CNetworkVehicleManager::Telemetry& CNetworkVehicleManager::GetTelemetry()
{
	return m_telemetry;
}

void CNetworkVehicleManager::UpdatePassenger(CVehicle* vehicle, CPlayerPed* localPlayer)
{
	CNetworkVehicle* networkVehicle = CNetworkVehicleManager::GetVehicle(vehicle);
	CPackets::VehiclePassengerUpdate* packet = CPacketHandler::VehiclePassengerUpdate__Collect(networkVehicle, localPlayer);
	CNetwork::SendPacket(CPacketsID::VEHICLE_PASSENGER_UPDATE, packet, sizeof * packet);
	delete packet;
}

unsigned char CNetworkVehicleManager::AddToTempList(CNetworkVehicle* networkVehicle)
{
	for (unsigned char i = 0; i < 255; i++)
	{
		if (m_apTempVehicles[i] == nullptr)
		{
			m_apTempVehicles[i] = networkVehicle;
			return i;
		}
	}

	return 255;
}

void CNetworkVehicleManager::RemoveHostedUnused()
{
	for (auto it = CNetworkVehicleManager::m_pVehicles.begin(); it != CNetworkVehicleManager::m_pVehicles.end();)
	{
		if ((*it)->m_bSyncing)
		{
			CVehicle* vehicle = (*it)->m_pVehicle;
			if (!IsVehiclePointerValid(vehicle))
			{
				delete* it;
				it = m_pVehicles.erase(it);
				continue;
			}
		}
		++it;
	}
}
