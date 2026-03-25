#pragma once
class CNetworkPedManager
{
public:
	struct InterpTuning
	{
		static constexpr size_t maxSnapshotBuffer = 32;
		static constexpr uint32_t interpolationDelayMs = 120;
		static constexpr float hardSnapDistance = 18.0f;
	};

	struct StreamConfig
	{
		float streamInRadius = 140.0f;
		float streamOutRadius = 170.0f;
		uint32_t tickIntervalMs = 300;
		size_t maxStreamed = 120;
	};

	struct Telemetry
	{
		size_t totalNetworkEntities = 0;
		size_t streamedEntities = 0;
		size_t pendingModelLoads = 0;
		size_t failedSpawns = 0;
		size_t streamChurnPerMinute = 0;
	};
	struct InterpStats
	{
		size_t correctionCount = 0;
		size_t bufferedSnapshots = 0;
	};

	static std::vector<CNetworkPed*> m_pPeds;
	static CNetworkPed* m_apTempPeds[255];
	static StreamConfig m_streamConfig;
	static Telemetry m_telemetry;
	static InterpStats m_interpStats;

	static CNetworkPed* GetPed(int pedid);
	static CNetworkPed* GetPed(CPed* ped);
	static CNetworkPed* GetPed(CEntity* entity);
	static void Add(CNetworkPed* ped);
	static void Remove(CNetworkPed* ped);
	static void Update();
	static void Process();
	static void AssignHost();
	static unsigned char AddToTempList(CNetworkPed* networkPed);
	static void RemoveHostedUnused();
	static void TickStreaming(uint32_t tickCount);
	static void CacheNetworkState(CNetworkPed* ped, const CPackets::PedOnFoot& packet);
	static const Telemetry& GetTelemetry();
	static const InterpStats& GetInterpStats();
};
