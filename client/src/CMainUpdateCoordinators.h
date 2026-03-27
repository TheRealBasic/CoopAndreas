#pragma once

#include "stdafx.h"
#include <array>

class CPlayerPed;

class CFrameStageProfiler {
public:
    enum class Stage : uint8_t {
        NetworkIngest,
        SyncEmit,
        RemoteEntityProcessing,
        HudDebugDraw,
        Count
    };

    struct StageStats {
        uint32_t lastDurationMs{};
        uint32_t maxDurationMs{};
        uint64_t totalDurationMs{};
        uint32_t samples{};

        uint32_t AverageDurationMs() const {
            return samples == 0 ? 0 : static_cast<uint32_t>(totalDurationMs / samples);
        }
    };

    static CFrameStageProfiler& Get();
    void Record(Stage stage, uint32_t durationMs);
    const StageStats& GetStats(Stage stage) const;

private:
    std::array<StageStats, static_cast<size_t>(Stage::Count)> m_stageStats{};
};

class CNetworkIngestCoordinator {
public:
    static void Process();
};

class CSyncEmitCoordinator {
public:
    static void ProcessConnectedFrame();

private:
    static void ProcessMissionSyncState();
    static void ProcessMissionFlagSync();
    static void ProcessLocalSyncEmit(CPlayerPed* localPlayer, uint32_t tickCount);
    static void ProcessAimSync(CPlayerPed* localPlayer, uint32_t tickCount);
};

class CRemoteEntityProcessingCoordinator {
public:
    static void Process();
};

class CHudDebugDrawCoordinator {
public:
    static void Process();
};
