#pragma once
class CNetworkPlayerManager
{
public:
	struct StreamingOverview
	{
		uint32_t lastTick = 0;
		size_t totalNetworkEntities = 0;
		size_t streamedEntities = 0;
		size_t pendingModelLoads = 0;
		size_t failedSpawns = 0;
		size_t streamChurnPerMinute = 0;
	};

	static std::vector<CNetworkPlayer*> m_pPlayers;
	static CPad m_pPads[MAX_SERVER_PLAYERS + 2];
	static int m_nMyId;
	static StreamingOverview m_streamingOverview;

	static void Add(CNetworkPlayer* player);
	static void Remove(CNetworkPlayer* player);
	static CNetworkPlayer* GetPlayer(CPlayerPed* playerPed);
	static CNetworkPlayer* GetPlayer(int playerid);
	static CNetworkPlayer* GetPlayer(CEntity* entity);
	static CNetworkPlayer* GetPlayer(CPed* ped);
	static void TickStreamingController(uint32_t tickCount);
};
