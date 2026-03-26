#include "stdafx.h"
#include "CNetworkVehicle.h"
#include "CNetworkPlayerManager.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>

std::vector<CNetworkVehicle*> CNetworkVehicleManager::m_pVehicles;
CNetworkVehicle* CNetworkVehicleManager::m_apTempVehicles[255];
CNetworkVehicleManager::StreamConfig CNetworkVehicleManager::m_streamConfig{};
CNetworkVehicleManager::Telemetry CNetworkVehicleManager::m_telemetry{};
CNetworkVehicleManager::InterpStats CNetworkVehicleManager::m_interpStats{};

static std::unordered_map<int, size_t> s_vehicleModelRefs;
static uint32_t s_lastVehicleStreamTick = 0;
static uint32_t s_streamChurnWindowStart = 0;
static size_t s_streamChurnCounter = 0;
static std::vector<CPackets::VehicleTrailerLinkSync> s_pendingTrailerLinkEvents;

namespace
{
	float Clamp01(float t)
	{
		return std::max(0.0f, std::min(1.0f, t));
	}

	CVector SlerpUnitVector(const CVector& from, const CVector& to, float t)
	{
		CVector a = from;
		CVector b = to;
		float aLen = a.Magnitude();
		float bLen = b.Magnitude();
		if (aLen < 0.0001f || bLen < 0.0001f)
			return from + (to - from) * t;
		a /= aLen;
		b /= bLen;

		float dot = std::max(-1.0f, std::min(1.0f, a.x * b.x + a.y * b.y + a.z * b.z));
		if (dot > 0.9995f)
			return (a + (b - a) * t);
		float theta = std::acos(dot) * t;
		CVector rel = b - a * dot;
		float relLen = rel.Magnitude();
		if (relLen > 0.0001f)
			rel /= relLen;
		return a * std::cos(theta) + rel * std::sin(theta);
	}

	float BasisVectorAngle(const CVector& a, const CVector& b)
	{
		float aLen = a.Magnitude();
		float bLen = b.Magnitude();
		if (aLen < 0.0001f || bLen < 0.0001f)
			return 0.0f;
		float dot = (a.x * b.x + a.y * b.y + a.z * b.z) / (aLen * bLen);
		dot = std::max(-1.0f, std::min(1.0f, dot));
		return std::acos(dot);
	}
}

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

void CNetworkVehicleManager::ProcessRemoteVehicles()
{
	ApplyPendingTrailerLinkEvents();

	m_interpStats.bufferedSnapshots = 0;
	m_interpStats.correctionCount = 0;
	const uint32_t now = GetTickCount();
	const uint32_t renderTick = now > CNetworkPlayerManager::InterpTuning::interpolationDelayMs ? now - CNetworkPlayerManager::InterpTuning::interpolationDelayMs : 0;

	for (auto* vehicle : m_pVehicles)
	{
		if (!vehicle || vehicle->m_bSyncing || !vehicle->m_pVehicle || !CUtil::IsValidEntityPtr(vehicle->m_pVehicle))
			continue;

		auto& snapshots = vehicle->m_interpSnapshots;
		if (snapshots.empty())
			continue;
		while (snapshots.size() > 2 && snapshots[1].arrivalTickMs <= renderTick)
			snapshots.pop_front();
		m_interpStats.bufferedSnapshots += snapshots.size();

		if (snapshots.size() == 1 || renderTick <= snapshots.front().arrivalTickMs)
		{
			const auto& s = snapshots.front();
			const float distanceError = (s.position - vehicle->m_pVehicle->m_matrix->pos).Magnitude();
			const float rotationError = std::max(BasisVectorAngle(vehicle->m_pVehicle->m_matrix->right, s.roll), BasisVectorAngle(vehicle->m_pVehicle->m_matrix->up, s.up));
			if (distanceError > CNetworkPlayerManager::InterpTuning::hardSnapDistance || rotationError > CNetworkPlayerManager::InterpTuning::hardSnapHeadingThresholdRad)
			{
				vehicle->m_nInterpCorrectionCount++;
				m_interpStats.correctionCount++;
			}
			vehicle->m_pVehicle->m_matrix->pos = s.position;
			vehicle->m_pVehicle->m_matrix->right = s.roll;
			vehicle->m_pVehicle->m_matrix->up = s.up;
			vehicle->m_pVehicle->m_vecMoveSpeed = s.velocity;
			vehicle->m_pVehicle->m_vecTurnSpeed = s.turnSpeed;
			vehicle->ApplyHydraulicsPacketState(s.hydraulicsControlState, 0, s.hydraulicsTransitionSequence, true);
			continue;
		}

		const auto& from = snapshots.front();
		const auto& to = snapshots[1];
		float span = static_cast<float>(to.arrivalTickMs - from.arrivalTickMs);
		float t = span > 0.0f ? Clamp01((renderTick - from.arrivalTickMs) / span) : 1.0f;
		const CVector targetPos = from.position + (to.position - from.position) * t;
		const CVector targetRight = SlerpUnitVector(from.roll, to.roll, t);
		const CVector targetUp = SlerpUnitVector(from.up, to.up, t);
		const float positionError = (targetPos - vehicle->m_pVehicle->m_matrix->pos).Magnitude();
		const float rotationError = std::max(BasisVectorAngle(vehicle->m_pVehicle->m_matrix->right, targetRight), BasisVectorAngle(vehicle->m_pVehicle->m_matrix->up, targetUp));

		if (positionError > CNetworkPlayerManager::InterpTuning::hardSnapDistance || rotationError > CNetworkPlayerManager::InterpTuning::hardSnapHeadingThresholdRad)
		{
			vehicle->m_nInterpCorrectionCount++;
			m_interpStats.correctionCount++;
			vehicle->m_pVehicle->m_matrix->pos = to.position;
			vehicle->m_pVehicle->m_matrix->right = to.roll;
			vehicle->m_pVehicle->m_matrix->up = to.up;
			vehicle->m_pVehicle->m_vecMoveSpeed = to.velocity;
			vehicle->m_pVehicle->m_vecTurnSpeed = to.turnSpeed;
			vehicle->ApplyHydraulicsPacketState(to.hydraulicsControlState, 0, to.hydraulicsTransitionSequence, true);
			continue;
		}

		if (positionError > CNetworkPlayerManager::InterpTuning::deadReckonPositionThreshold)
		{
			const float posBlend = t * CNetworkPlayerManager::InterpTuning::translationSmoothFactor;
			vehicle->m_pVehicle->m_matrix->pos = vehicle->m_pVehicle->m_matrix->pos + (targetPos - vehicle->m_pVehicle->m_matrix->pos) * posBlend;
		}
		if (rotationError > CNetworkPlayerManager::InterpTuning::deadReckonRotationThresholdRad)
		{
			const float rotBlend = t * CNetworkPlayerManager::InterpTuning::rotationSmoothFactor;
			vehicle->m_pVehicle->m_matrix->right = SlerpUnitVector(vehicle->m_pVehicle->m_matrix->right, targetRight, rotBlend);
			vehicle->m_pVehicle->m_matrix->up = SlerpUnitVector(vehicle->m_pVehicle->m_matrix->up, targetUp, rotBlend);
		}
		vehicle->m_pVehicle->m_vecMoveSpeed = from.velocity + (to.velocity - from.velocity) * t;
		vehicle->m_pVehicle->m_vecTurnSpeed = from.turnSpeed + (to.turnSpeed - from.turnSpeed) * t;
		const uint8_t interpHydraulicsControlState = (t < 0.5f) ? from.hydraulicsControlState : to.hydraulicsControlState;
		const uint16_t interpHydraulicsTransitionSequence = (t < 0.5f) ? from.hydraulicsTransitionSequence : to.hydraulicsTransitionSequence;
		vehicle->ApplyHydraulicsPacketState(interpHydraulicsControlState, 0, interpHydraulicsTransitionSequence, false);
	}
}

void CNetworkVehicleManager::QueueTrailerLinkEvent(const CPackets::VehicleTrailerLinkSync& packet)
{
	s_pendingTrailerLinkEvents.push_back(packet);
}

void CNetworkVehicleManager::ApplyPendingTrailerLinkEvents()
{
	if (s_pendingTrailerLinkEvents.empty())
		return;

	std::stable_sort(s_pendingTrailerLinkEvents.begin(), s_pendingTrailerLinkEvents.end(),
		[](const CPackets::VehicleTrailerLinkSync& a, const CPackets::VehicleTrailerLinkSync& b)
		{
			if (a.linkVersion != b.linkVersion)
				return a.linkVersion < b.linkVersion;
			if (a.timestampMs != b.timestampMs)
				return a.timestampMs < b.timestampMs;
			if (a.tractorVehicleId != b.tractorVehicleId)
				return a.tractorVehicleId < b.tractorVehicleId;
			return a.trailerVehicleId < b.trailerVehicleId;
		});

	for (const auto& packet : s_pendingTrailerLinkEvents)
	{
		CNetworkVehicle* tractor = CNetworkVehicleManager::GetVehicle(packet.tractorVehicleId);
		CNetworkVehicle* trailer = CNetworkVehicleManager::GetVehicle(packet.trailerVehicleId);
		if (!tractor || !trailer || !tractor->m_pVehicle || !trailer->m_pVehicle)
			continue;
		if (!CUtil::IsValidEntityPtr(tractor->m_pVehicle) || !CUtil::IsValidEntityPtr(trailer->m_pVehicle))
			continue;

		CVehicle* tractorEntity = tractor->m_pVehicle;
		CVehicle* trailerEntity = trailer->m_pVehicle;
		if (packet.attachState)
		{
			tractorEntity->m_pTrailer = trailerEntity;
			trailerEntity->m_pTractor = tractorEntity;
		}
		else
		{
			if (tractorEntity->m_pTrailer == trailerEntity)
				tractorEntity->m_pTrailer = nullptr;
			if (trailerEntity->m_pTractor == tractorEntity)
				trailerEntity->m_pTractor = nullptr;
		}
	}

	s_pendingTrailerLinkEvents.clear();
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
	vehicle->ApplyHydraulicsPacketState(packet.hydraulicsControlState, packet.hydraulicsTransitionMask, packet.hydraulicsTransitionSequence, false);

	CNetworkVehicle::InterpSnapshot snapshot{};
	snapshot.position = packet.pos;
	snapshot.roll = packet.roll;
	snapshot.up = packet.rot;
	snapshot.velocity = packet.velocity;
	snapshot.turnSpeed = packet.turnSpeed;
	snapshot.health = packet.health;
	snapshot.hydraulicsControlState = packet.hydraulicsControlState;
	snapshot.hydraulicsTransitionSequence = packet.hydraulicsTransitionSequence;
	snapshot.arrivalTickMs = GetTickCount();
	snapshot.serverSequence = ++vehicle->m_nSnapshotSequence;
	vehicle->m_interpSnapshots.push_back(snapshot);
	while (vehicle->m_interpSnapshots.size() > InterpTuning::maxSnapshotBuffer)
		vehicle->m_interpSnapshots.pop_front();
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
	vehicle->ApplyHydraulicsPacketState(packet.hydraulicsControlState, packet.hydraulicsTransitionMask, packet.hydraulicsTransitionSequence, false);

	CNetworkVehicle::InterpSnapshot snapshot{};
	snapshot.position = packet.pos;
	snapshot.roll = packet.roll;
	snapshot.up = packet.rot;
	snapshot.velocity = packet.velocity;
	snapshot.turnSpeed = vehicle->m_cachedState.turnSpeed;
	snapshot.health = packet.health;
	snapshot.hydraulicsControlState = packet.hydraulicsControlState;
	snapshot.hydraulicsTransitionSequence = packet.hydraulicsTransitionSequence;
	snapshot.arrivalTickMs = GetTickCount();
	snapshot.serverSequence = ++vehicle->m_nSnapshotSequence;
	vehicle->m_interpSnapshots.push_back(snapshot);
	while (vehicle->m_interpSnapshots.size() > InterpTuning::maxSnapshotBuffer)
		vehicle->m_interpSnapshots.pop_front();
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

const CNetworkVehicleManager::InterpStats& CNetworkVehicleManager::GetInterpStats()
{
	return m_interpStats;
}

void CNetworkVehicleManager::UpdatePassenger(CVehicle* vehicle, CPlayerPed* localPlayer)
{
	CNetworkVehicle* networkVehicle = CNetworkVehicleManager::GetVehicle(vehicle);
	CPackets::VehiclePassengerUpdate* packet = CPacketHandler::VehiclePassengerUpdate__Collect(networkVehicle, localPlayer);
	if (packet == nullptr)
		return;
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
