#pragma once
class CNetworkVehicleManager
{
public:
    struct InterpTuning
    {
        static constexpr size_t maxSnapshotBuffer = 32;
        static constexpr uint32_t interpolationDelayMs = 120;
        static constexpr float hardSnapDistance = 30.0f;
    };

    struct StreamConfig
    {
        float streamInRadius = 180.0f;
        float streamOutRadius = 220.0f;
        uint32_t tickIntervalMs = 300;
        size_t maxStreamed = 80;
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

    static std::vector<CNetworkVehicle*> m_pVehicles;
    static CNetworkVehicle* m_apTempVehicles[255];
    static StreamConfig m_streamConfig;
    static Telemetry m_telemetry;
    static InterpStats m_interpStats;

    static CNetworkVehicle* GetVehicle(int vehicleid);
    static CNetworkVehicle* GetVehicle(CVehicle* vehicle);
    static CNetworkVehicle* GetVehicle(CEntity* vehicle);
    static void Add(CNetworkVehicle* vehicle);
    static void Remove(CNetworkVehicle* vehicle);
    static void UpdateDriver(CVehicle* vehicle);
    static void UpdateIdle();
    static void ProcessRemoteVehicles();
    static void UpdatePassenger(CVehicle* vehicle, CPlayerPed* localPlayer);
    static unsigned char AddToTempList(CNetworkVehicle* networkVehicle);
    static void RemoveHostedUnused();
    static void TickStreaming(uint32_t tickCount);
    static void CacheNetworkState(CNetworkVehicle* vehicle, const CPackets::VehicleIdleUpdate& packet);
    static void CacheNetworkState(CNetworkVehicle* vehicle, const CPackets::VehicleDriverUpdate& packet);
    static void AddModelRef(int modelId);
    static void ReleaseModelRef(int modelId);
    static const Telemetry& GetTelemetry();
    static const InterpStats& GetInterpStats();
};
