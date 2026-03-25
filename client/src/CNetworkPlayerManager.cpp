#include "stdafx.h"

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
}
