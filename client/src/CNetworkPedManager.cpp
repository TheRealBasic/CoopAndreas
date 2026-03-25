#include "stdafx.h"
#include "CNetworkVehicle.h"
#include "CNetworkPed.h"

std::vector<CNetworkPed*> CNetworkPedManager::m_pPeds;
CNetworkPed* CNetworkPedManager::m_apTempPeds[255];
CNetworkPedManager::StreamConfig CNetworkPedManager::m_streamConfig{};
CNetworkPedManager::Telemetry CNetworkPedManager::m_telemetry{};

static uint32_t s_lastPedStreamTick = 0;
static uint32_t s_pedChurnWindowStart = 0;
static size_t s_pedChurnCounter = 0;

CNetworkPed* CNetworkPedManager::GetPed(int pedid)
{
	for (int i = 0; i != m_pPeds.size(); i++)
	{
		if (m_pPeds[i]->m_nPedId == pedid)
		{
			return m_pPeds[i];
		}
	}
	return nullptr;
}

CNetworkPed* CNetworkPedManager::GetPed(CPed* ped)
{
	if (ped == nullptr)
		return nullptr;

	for (int i = 0; i != m_pPeds.size(); i++)
	{
		if (m_pPeds[i]->m_pPed == ped)
		{
			return m_pPeds[i];
		}
	}

	return nullptr;
}

CNetworkPed* CNetworkPedManager::GetPed(CEntity* entity)
{
	if (entity == nullptr)
		return nullptr;

	for (int i = 0; i != m_pPeds.size(); i++)
	{
		if (m_pPeds[i]->m_pPed == entity)
		{
			return m_pPeds[i];
		}
	}

	return nullptr;
}

void CNetworkPedManager::Add(CNetworkPed* ped)
{
	CNetworkPedManager::m_pPeds.push_back(ped);
}

void CNetworkPedManager::Remove(CNetworkPed* ped)
{
	auto it = std::find(m_pPeds.begin(), m_pPeds.end(), ped);
	if (it != m_pPeds.end())
	{
		m_pPeds.erase(it);
	}
}

void CNetworkPedManager::Update()
{
	CNetworkPedManager::RemoveHostedUnused();
	TickStreaming(GetTickCount());

	CMassPacketBuilder builder;

	for (CNetworkPed* networkPed : m_pPeds)
	{
		if (!networkPed->m_bSyncing)
			continue;

		CPed* ped = networkPed->m_pPed;
		if (!ped) continue;

		CVehicle* vehicle = ped->m_pVehicle;
		CNetworkVehicle* networkVehicle = vehicle ? CNetworkVehicleManager::GetVehicle(vehicle) : nullptr;

		if (networkVehicle && ped->m_nPedFlags.bInVehicle)
		{
			bool isDriver = (vehicle->m_pDriver == ped);

			if (isDriver)
			{
				auto pedDriverUpdatePacket = CPacketHandler::PedDriverUpdate__Collect(networkVehicle, networkPed);
				builder.AddPacket(CPacketsID::PED_DRIVER_UPDATE, pedDriverUpdatePacket, sizeof(*pedDriverUpdatePacket));
				delete pedDriverUpdatePacket;
			}
			else
			{
				auto pedPassengerSyncPacket = CPacketHandler::PedPassengerSync__Collect(networkPed, networkVehicle);
				builder.AddPacket(CPacketsID::PED_PASSENGER_UPDATE, pedPassengerSyncPacket, sizeof(*pedPassengerSyncPacket));
				delete pedPassengerSyncPacket;
			}
		}
		else
		{
			auto pedOnFootPacket = CPacketHandler::PedOnFoot__Collect(networkPed);
			builder.AddPacket(CPacketsID::PED_ONFOOT, pedOnFootPacket, sizeof(*pedOnFootPacket));
			delete pedOnFootPacket;
		}
	}

	builder.Send();
}

void CNetworkPedManager::Process()
{
	for (auto networkPed : m_pPeds)
	{
		if (networkPed->m_bSyncing)
			continue;

		CPed* ped = networkPed->m_pPed;

		if (ped == nullptr)
			continue;

		ped->m_fAimingRotation = networkPed->m_fAimingRotation;
		ped->m_fCurrentRotation = networkPed->m_fCurrentRotation;
		ped->field_73C = networkPed->m_fLookDirection;
	}
}

void CNetworkPedManager::TickStreaming(uint32_t tickCount)
{
	if (tickCount < s_lastPedStreamTick + m_streamConfig.tickIntervalMs)
		return;

	s_lastPedStreamTick = tickCount;
	if (!FindPlayerPed(0) || !FindPlayerPed(0)->m_matrix)
		return;

	CVector localPos = FindPlayerPed(0)->m_matrix->pos;
	size_t streamedNow = 0;

	for (auto* ped : m_pPeds)
	{
		if (!ped)
			continue;

		bool shouldStayStreamed = ped->m_bMissionCritical;
		CVector delta = ped->m_cachedState.position - localPos;
		float distance = delta.Magnitude();
		if (!shouldStayStreamed)
		{
			if (ped->m_eStreamState == CNetworkPed::eStreamState::Streamed)
				shouldStayStreamed = distance <= m_streamConfig.streamOutRadius;
			else
				shouldStayStreamed = distance <= m_streamConfig.streamInRadius;
		}

		if (shouldStayStreamed && streamedNow < m_streamConfig.maxStreamed)
		{
			if (!ped->m_pPed)
			{
				if (!ped->StreamIn())
					m_telemetry.failedSpawns++;
				s_pedChurnCounter++;
			}
			streamedNow++;
		}
		else if (ped->m_pPed)
		{
			ped->StreamOut();
			s_pedChurnCounter++;
		}
	}

	if (s_pedChurnWindowStart == 0)
		s_pedChurnWindowStart = tickCount;
	if (tickCount - s_pedChurnWindowStart >= 60000)
	{
		m_telemetry.streamChurnPerMinute = s_pedChurnCounter;
		s_pedChurnCounter = 0;
		s_pedChurnWindowStart = tickCount;
	}

	m_telemetry.totalNetworkEntities = m_pPeds.size();
	m_telemetry.streamedEntities = streamedNow;
	m_telemetry.pendingModelLoads = 0;
}

void CNetworkPedManager::CacheNetworkState(CNetworkPed* ped, const CPackets::PedOnFoot& packet)
{
	if (!ped)
		return;

	ped->m_cachedState.position = packet.pos;
	ped->m_cachedState.velocity = packet.velocity;
	ped->m_cachedState.aimingRotation = packet.aimingRotation;
	ped->m_cachedState.currentRotation = packet.currentRotation;
	ped->m_cachedState.lookDirection = packet.lookDirection;
	ped->m_cachedState.health = packet.health;
	ped->m_cachedState.armour = packet.armour;
	ped->m_cachedState.weapon = packet.weapon;
	ped->m_cachedState.ammo = packet.ammo;
}

const CNetworkPedManager::Telemetry& CNetworkPedManager::GetTelemetry()
{
	return m_telemetry;
}

void CNetworkPedManager::AssignHost()
{
	/*for (auto networkPed : m_pPeds)
	{
		CPed* ped = networkPed->m_pPed;

		if (ped)
		{
			ped->SetCharCreatedBy(networkPed->m_nCreatedBy);

			CTaskComplexWander* task = plugin::CallAndReturn<CTaskComplexWander*, 0x673D00>(ped); // GetWanderTaskByPedType
			ped->m_pIntelligence->m_TaskMgr.SetTask(task, 0, false);
		}
	}*/
}

unsigned char CNetworkPedManager::AddToTempList(CNetworkPed* networkPed)
{
	for (unsigned char i = 0; i < 255; i++)
	{
		if (m_apTempPeds[i] == nullptr)
		{
			m_apTempPeds[i] = networkPed;
			return i;
		}
	}

	return 255;
}

void CNetworkPedManager::RemoveHostedUnused()
{
	for (auto it = CNetworkPedManager::m_pPeds.begin(); it != CNetworkPedManager::m_pPeds.end();)
	{
		if ((*it)->m_bSyncing)
		{
			CPed* ped = (*it)->m_pPed;
			if (!IsPedPointerValid(ped))
			{
				delete* it;
				it = m_pPeds.erase(it);
				continue;
			}
		}
		++it;
	}
}
