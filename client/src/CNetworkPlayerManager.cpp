#include "stdafx.h"
#include <algorithm>
#include <cmath>

namespace
{
	float Clamp01(float t)
	{
		return std::max(0.0f, std::min(1.0f, t));
	}

	float LerpAngle(float a, float b, float t)
	{
		constexpr float kPi = 3.14159265358979323846f;
		constexpr float kTwoPi = 6.28318530717958647692f;
		float delta = b - a;
		while (delta > kPi) delta -= kTwoPi;
		while (delta < -kPi) delta += kTwoPi;
		return a + delta * t;
	}
}

std::vector<CNetworkPlayer*> CNetworkPlayerManager::m_pPlayers;
CPad CNetworkPlayerManager::m_pPads[MAX_SERVER_PLAYERS + 2];
int CNetworkPlayerManager::m_nMyId;
CNetworkPlayerManager::StreamingOverview CNetworkPlayerManager::m_streamingOverview{};

void CNetworkPlayerManager::Add(CNetworkPlayer* player)
{
	m_pPlayers.push_back(player);
}

void CNetworkPlayerManager::Remove(CNetworkPlayer* player)
{
	auto it = std::find(m_pPlayers.begin(), m_pPlayers.end(), player);
	if (it != m_pPlayers.end())
	{
		m_pPlayers.erase(it);
	}
}

CNetworkPlayer* CNetworkPlayerManager::GetPlayer(CPlayerPed* playerPed)
{
	for (int i = 0; i != m_pPlayers.size(); i++)
	{
		if (m_pPlayers[i]->m_pPed == playerPed)
		{
			return m_pPlayers[i];
		}
	}
	return nullptr;
}

CNetworkPlayer* CNetworkPlayerManager::GetPlayer(CPed* ped)
{
	for (int i = 0; i != m_pPlayers.size(); i++)
	{
		if (m_pPlayers[i]->m_pPed == ped)
		{
			return m_pPlayers[i];
		}
	}
	return nullptr;
}

CNetworkPlayer* CNetworkPlayerManager::GetPlayer(int playerid)
{
	for (int i = 0; i != m_pPlayers.size(); i++)
	{
		if (m_pPlayers[i]->m_iPlayerId == playerid)
		{
			return m_pPlayers[i];
		}
	}
	return nullptr;
}

CNetworkPlayer* CNetworkPlayerManager::GetPlayer(CEntity* entity)
{
	for (int i = 0; i != m_pPlayers.size(); i++)
	{
		if (m_pPlayers[i]->m_pPed == entity)
		{
			return m_pPlayers[i];
		}
	}
	return nullptr;
}

void CNetworkPlayerManager::TickStreamingController(uint32_t tickCount)
{
	if (tickCount < m_streamingOverview.lastTick + 250)
		return;

	m_streamingOverview.lastTick = tickCount;
	CNetworkVehicleManager::TickStreaming(tickCount);
	CNetworkPedManager::TickStreaming(tickCount);

	const auto& vehicleTelemetry = CNetworkVehicleManager::GetTelemetry();
	const auto& pedTelemetry = CNetworkPedManager::GetTelemetry();

	m_streamingOverview.totalNetworkEntities = vehicleTelemetry.totalNetworkEntities + pedTelemetry.totalNetworkEntities;
	m_streamingOverview.streamedEntities = vehicleTelemetry.streamedEntities + pedTelemetry.streamedEntities;
	m_streamingOverview.pendingModelLoads = vehicleTelemetry.pendingModelLoads + pedTelemetry.pendingModelLoads;
	m_streamingOverview.failedSpawns = vehicleTelemetry.failedSpawns + pedTelemetry.failedSpawns;
	m_streamingOverview.streamChurnPerMinute = vehicleTelemetry.streamChurnPerMinute + pedTelemetry.streamChurnPerMinute;
	m_streamingOverview.pedInterpCorrections = CNetworkPedManager::GetInterpStats().correctionCount;
	m_streamingOverview.vehicleInterpCorrections = CNetworkVehicleManager::GetInterpStats().correctionCount;
	m_streamingOverview.pedSnapshotBacklog = CNetworkPedManager::GetInterpStats().bufferedSnapshots;
	m_streamingOverview.vehicleSnapshotBacklog = CNetworkVehicleManager::GetInterpStats().bufferedSnapshots;
}

void CNetworkPlayerManager::ProcessRemotePlayers()
{
	m_streamingOverview.playerInterpCorrections = 0;
	m_streamingOverview.playerSnapshotBacklog = 0;
	const uint32_t renderTick = GetTickCount() > InterpTuning::interpolationDelayMs ? GetTickCount() - InterpTuning::interpolationDelayMs : 0;

	for (auto* player : m_pPlayers)
	{
		if (!player || !player->m_pPed || player->m_pPed->m_nPedFlags.bInVehicle)
			continue;

		auto& snapshots = player->m_interpSnapshots;
		if (snapshots.empty())
			continue;

		while (snapshots.size() > 2 && snapshots[1].arrivalTickMs <= renderTick)
			snapshots.pop_front();

		m_streamingOverview.playerSnapshotBacklog += snapshots.size();

		auto applySnapshot = [&](const CNetworkPlayer::InterpSnapshot& snapshot)
		{
			if ((snapshot.position - player->m_pPed->m_matrix->pos).Magnitude() > InterpTuning::hardSnapDistance)
			{
				player->m_nInterpCorrectionCount++;
				m_streamingOverview.playerInterpCorrections++;
			}

			player->m_pPed->m_matrix->pos = snapshot.position;
			player->m_pPed->m_fCurrentRotation = snapshot.currentRotation;
			player->m_pPed->m_fAimingRotation = snapshot.aimingRotation;
			player->m_pPed->m_vecMoveSpeed = snapshot.velocity;
		};

		if (snapshots.size() == 1 || renderTick <= snapshots.front().arrivalTickMs)
		{
			applySnapshot(snapshots.front());
			continue;
		}

		const auto& from = snapshots.front();
		const auto& to = snapshots[1];
		float denom = static_cast<float>(to.arrivalTickMs - from.arrivalTickMs);
		float t = denom > 0.0f ? Clamp01((renderTick - from.arrivalTickMs) / denom) : 1.0f;

		CVector pos = from.position + (to.position - from.position) * t;
		if ((pos - player->m_pPed->m_matrix->pos).Magnitude() > InterpTuning::hardSnapDistance)
		{
			player->m_nInterpCorrectionCount++;
			m_streamingOverview.playerInterpCorrections++;
			pos = to.position;
			t = 1.0f;
		}

		player->m_pPed->m_matrix->pos = pos;
		player->m_pPed->m_fCurrentRotation = LerpAngle(from.currentRotation, to.currentRotation, t);
		player->m_pPed->m_fAimingRotation = LerpAngle(from.aimingRotation, to.aimingRotation, t);
		player->m_pPed->m_vecMoveSpeed = from.velocity + (to.velocity - from.velocity) * t;
	}
}
