#include "stdafx.h"
#include "CWantedSync.h"

namespace
{
	int g_nStoredWantedLevel = 0;
	bool g_bHasStoredWantedLevel = false;
}

void CWantedSync::ApplyNetworkPlayerContext(CNetworkPlayer* player)
{
	const int focusPlayerIndex = CWorld::PlayerInFocus;
	if (focusPlayerIndex < 0 || focusPlayerIndex >= MAX_PLAYERS)
	{
		return;
	}

	auto* playerInfo = &CWorld::Players[focusPlayerIndex];
	if (!playerInfo || !playerInfo->m_pWanted)
	{
		return;
	}

	g_nStoredWantedLevel = playerInfo->m_pWanted->m_nWantedLevel;
	g_bHasStoredWantedLevel = true;
	playerInfo->m_pWanted->m_nWantedLevel = player->m_stats.GetWantedLevel();
}

void CWantedSync::ApplyLocalContext()
{
	if (!g_bHasStoredWantedLevel)
	{
		return;
	}

	const int focusPlayerIndex = CWorld::PlayerInFocus;
	if (focusPlayerIndex < 0 || focusPlayerIndex >= MAX_PLAYERS)
	{
		g_bHasStoredWantedLevel = false;
		return;
	}

	auto* playerInfo = &CWorld::Players[focusPlayerIndex];
	if (playerInfo && playerInfo->m_pWanted)
	{
		playerInfo->m_pWanted->m_nWantedLevel = g_nStoredWantedLevel;
	}

	g_bHasStoredWantedLevel = false;
}
