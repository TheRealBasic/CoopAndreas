#pragma once
class CNetworkPlayerManager
{
public:
	struct InterpTuning
	{
		static constexpr size_t maxSnapshotBuffer = 32;
		static constexpr uint32_t interpolationDelayMs = 120;
		static constexpr float hardSnapDistance = 22.0f;
	};

	struct StreamingOverview
	{
		uint32_t lastTick = 0;
		size_t totalNetworkEntities = 0;
		size_t streamedEntities = 0;
		size_t pendingModelLoads = 0;
		size_t failedSpawns = 0;
		size_t streamChurnPerMinute = 0;
		size_t playerInterpCorrections = 0;
		size_t pedInterpCorrections = 0;
		size_t vehicleInterpCorrections = 0;
		size_t playerSnapshotBacklog = 0;
		size_t pedSnapshotBacklog = 0;
		size_t vehicleSnapshotBacklog = 0;
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
	static void ProcessRemotePlayers();
};
