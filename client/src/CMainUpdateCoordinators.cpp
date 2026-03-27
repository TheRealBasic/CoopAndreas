#include "CMainUpdateCoordinators.h"

#include "CDXFont.h"
#include "CAimSync.h"
#include "CChat.h"
#include "CCore.h"
#include "CDebugPedTasks.h"
#include "CDebugVehicleSpawner.h"
#include "CDiscordRPCMgr.h"
#include "CDriveBy.h"
#include "CEntryExitManager.h"
#include "CEntryExitMarkerSync.h"
#include "CLocalPlayer.h"
#include "CMissionSyncState.h"
#include "CNetwork.h"
#include "CNetworkAnimQueue.h"
#include "CNetworkCheckpoint.h"
#include "CNetworkPedManager.h"
#include "CNetworkPickupManager.h"
#include "CNetworkPlayerList.h"
#include "CNetworkPlayerManager.h"
#include "CNetworkPlayerMapPin.h"
#include "CNetworkPlayerNameTag.h"
#include "CNetworkPlayerWaypoint.h"
#include "CNetworkStaticBlip.h"
#include "CNetworkVehicleManager.h"
#include "CPacketHandler.h"
#include "CPassengerEnter.h"
#include "CSniperAimMarkerSync.h"
#include "CStatsSync.h"
#include "CUtil.h"
#include <game_sa/CTagManager.h>
#include <semver.h>

namespace {
    struct LocalPlayerContext {
        CPlayerPed* player{};
        uint32_t tickCount{};
    };

    enum class TickSkipReason : uint8_t {
        LocalPlayerMissing,
        LocalPlayerInvalid,
        LocalPlayerIntelligenceMissing,
        LocalPlayerUnavailable,
        Count
    };

    unsigned int g_lastOnFootSyncTickRate = 0;
    unsigned int g_lastDriverSyncTickRate = 0;
    unsigned int g_lastIdleVehicleSyncTickRate = 0;
    unsigned int g_lastPassengerSyncTickRate = 0;
    unsigned int g_lastPedSyncTickRate = 0;
    unsigned int g_lastWeatherTimeSyncTickRate = 0;
    unsigned int g_lastPlayerAimSyncTickRate = 0;

    bool g_hasBeenConnected = false;
    bool g_lastOnMissionFlag = false;
    uint32_t g_networkWindowStartTime = 0;

    const char* StageName(CFrameStageProfiler::Stage stage) {
        switch (stage) {
        case CFrameStageProfiler::Stage::NetworkIngest: return "ingest";
        case CFrameStageProfiler::Stage::SyncEmit: return "sync";
        case CFrameStageProfiler::Stage::RemoteEntityProcessing: return "remote";
        case CFrameStageProfiler::Stage::HudDebugDraw: return "hud";
        default: return "unknown";
        }
    }

#if DEBUG
    std::array<uint64_t, static_cast<size_t>(TickSkipReason::Count)> g_tickSkipCounters{};

    void RecordTickSkip(TickSkipReason reason) {
        ++g_tickSkipCounters[static_cast<size_t>(reason)];
    }

    const char* TickSkipReasonName(TickSkipReason reason) {
        switch (reason) {
        case TickSkipReason::LocalPlayerMissing: return "ctx.missing";
        case TickSkipReason::LocalPlayerInvalid: return "ctx.invalid";
        case TickSkipReason::LocalPlayerIntelligenceMissing: return "ctx.intel";
        case TickSkipReason::LocalPlayerUnavailable: return "ctx.unavailable";
        default: return "ctx.unknown";
        }
    }
#else
    void RecordTickSkip(TickSkipReason) {
    }
#endif

    bool IsLocalPlayerUnavailableForSync(const CPlayerPed* player) {
        return !player
            || player->m_fHealth <= 0.0f
            || player->m_ePedState == PEDSTATE_ARRESTED
            || player->m_ePedState == PEDSTATE_DEAD
            || (player->m_ePedState == PEDSTATE_DIE && player->m_nPedFlags.bIsDyingStuck)
            || CCutsceneMgr::ms_running;
    }

    bool TryGetLocalPlayerContext(LocalPlayerContext& outContext) {
        CPlayerPed* localPlayer = FindPlayerPed(0);
        if (!localPlayer) {
            RecordTickSkip(TickSkipReason::LocalPlayerMissing);
            return false;
        }

        if (!CUtil::IsValidEntityPtr(localPlayer)) {
            RecordTickSkip(TickSkipReason::LocalPlayerInvalid);
            return false;
        }

        if (!localPlayer->m_pIntelligence) {
            RecordTickSkip(TickSkipReason::LocalPlayerIntelligenceMissing);
            return false;
        }

        if (IsLocalPlayerUnavailableForSync(localPlayer)) {
            RecordTickSkip(TickSkipReason::LocalPlayerUnavailable);
            return false;
        }

        outContext.player = localPlayer;
        outContext.tickCount = GetTickCount();
        return true;
    }
}

CFrameStageProfiler& CFrameStageProfiler::Get() {
    static CFrameStageProfiler s_profiler;
    return s_profiler;
}

void CFrameStageProfiler::Record(Stage stage, uint32_t durationMs) {
    auto& stats = m_stageStats[static_cast<size_t>(stage)];
    stats.lastDurationMs = durationMs;
    stats.maxDurationMs = std::max(stats.maxDurationMs, durationMs);
    stats.totalDurationMs += durationMs;
    ++stats.samples;
}

const CFrameStageProfiler::StageStats& CFrameStageProfiler::GetStats(Stage stage) const {
    return m_stageStats[static_cast<size_t>(stage)];
}

void CNetworkIngestCoordinator::Process() {
    const uint32_t stageStart = GetTickCount();

    if (CNetwork::m_bConnected)
    {
        g_hasBeenConnected = true;

        ENetEvent event;
        while (enet_host_service(CNetwork::m_pClient, &event, 1) != 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
            {
                CNetwork::ms_nBytesReceivedThisSecondCounter += event.packet->dataLength;
                CNetwork::HandlePacketReceive(event);
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                CNetwork::Disconnect();
                break;
            }
            default:
                break;
            }
        }

        const uint32_t currentTime = GetTickCount();
        if (currentTime - g_networkWindowStartTime >= 1000)
        {
            CNetwork::ms_nBytesReceivedThisSecond = CNetwork::ms_nBytesReceivedThisSecondCounter;
            CNetwork::ms_nBytesSentThisSecond = CNetwork::ms_nBytesSentThisSecondCounter;
            CNetwork::ms_nBytesReceivedThisSecondCounter = 0;
            CNetwork::ms_nBytesSentThisSecondCounter = 0;
            g_networkWindowStartTime = currentTime;
        }
    }
    else if (g_hasBeenConnected)
    {
        g_hasBeenConnected = false;
        CNetwork::Shutdown();
        CChat::AddMessage("{cecedb}[Network] Disconnected from the server.");
    }

    CFrameStageProfiler::Get().Record(CFrameStageProfiler::Stage::NetworkIngest, GetTickCount() - stageStart);
}

void CSyncEmitCoordinator::ProcessMissionSyncState() {
    CMissionSyncState::ProcessDeferredCutsceneStart();
    CMissionSyncState::ProcessMissionAudioLoading();
    CMissionSyncState::ProcessWidescreenPolicy();
    CMissionSyncState::ProcessSubmissionMissionSync();
}

void CSyncEmitCoordinator::ProcessMissionFlagSync() {
    if (CLocalPlayer::m_bIsHost
        && CTheScripts::OnAMissionFlag
        && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag] != g_lastOnMissionFlag)
    {
        g_lastOnMissionFlag = CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag];
        CPacketHandler::OnMissionFlagSync__Trigger();
    }
}

void CSyncEmitCoordinator::ProcessAimSync(CPlayerPed* localPlayer, uint32_t tickCount) {
    if (localPlayer->m_pIntelligence->GetTaskUseGun())
    {
        if (tickCount > g_lastPlayerAimSyncTickRate + 50)
        {
            CPacketHandler::PlayerAimSync__Trigger();
            g_lastPlayerAimSyncTickRate = tickCount;
        }
    }
    else if (localPlayer->m_pVehicle && CUtil::IsVehicleHasTurret(localPlayer->m_pVehicle))
    {
        if (tickCount > g_lastPlayerAimSyncTickRate + 150)
        {
            CPacketHandler::PlayerAimSync__Trigger();
            g_lastPlayerAimSyncTickRate = tickCount;
        }
    }
    else if (localPlayer->m_pIntelligence->GetTaskSwim() || localPlayer->m_pIntelligence->GetTaskJetPack() || localPlayer->m_pIntelligence->GetTaskFighting())
    {
        if (tickCount > g_lastPlayerAimSyncTickRate + 200)
        {
            CPacketHandler::PlayerAimSync__Trigger();
            g_lastPlayerAimSyncTickRate = tickCount;
        }
    }
}

void CSyncEmitCoordinator::ProcessLocalSyncEmit(CPlayerPed* localPlayer, uint32_t tickCount) {
    CNetworkPlayerManager::TickStreamingController(tickCount);

    CPassengerEnter::Process();

    CDriveBy::Process(localPlayer);
    CStatsSync::NotifyWantedLevelChanged();

    int syncRate = 40;

    const bool isDriver = localPlayer->m_nPedFlags.bInVehicle && localPlayer->m_pVehicle && localPlayer->m_pVehicle->m_pDriver == localPlayer;
    const bool isPassenger = localPlayer->m_nPedFlags.bInVehicle && localPlayer->m_pVehicle && localPlayer->m_pVehicle->m_pDriver != localPlayer;

    const CVector velocity = isDriver ? localPlayer->m_pVehicle->m_vecMoveSpeed : localPlayer->m_vecMoveSpeed;

    if (velocity.x == 0 && velocity.y == 0 && velocity.z == 0)
    {
        syncRate = 110;
    }

    if (!isPassenger && tickCount > (isDriver ? g_lastDriverSyncTickRate : g_lastOnFootSyncTickRate) + syncRate)
    {
        if (isDriver)
        {
            CNetworkVehicleManager::UpdateDriver(localPlayer->m_pVehicle);
            g_lastDriverSyncTickRate = tickCount;
        }
        else
        {
            CPackets::PlayerOnFoot packet = CPacketHandler::PlayerOnFoot__Collect();
            CNetwork::SendPacket(CPacketsID::PLAYER_ONFOOT, &packet, sizeof(packet));
            g_lastOnFootSyncTickRate = tickCount;
        }
    }

    if (isPassenger && tickCount > g_lastPassengerSyncTickRate + 333)
    {
        CNetworkVehicleManager::UpdatePassenger(localPlayer->m_pVehicle, localPlayer);
        g_lastPassengerSyncTickRate = tickCount;
    }

    if (tickCount > g_lastIdleVehicleSyncTickRate + 150)
    {
        CNetworkVehicleManager::UpdateIdle();
        g_lastIdleVehicleSyncTickRate = tickCount;
    }

    if (tickCount > g_lastPedSyncTickRate + 50)
    {
        CNetworkPedManager::Update();
        g_lastPedSyncTickRate = tickCount;
    }

    if (CLocalPlayer::m_bIsHost && tickCount > g_lastWeatherTimeSyncTickRate + 2000)
    {
        CPacketHandler::GameWeatherTime__Trigger();
        g_lastWeatherTimeSyncTickRate = tickCount;
    }

    if (CLocalPlayer::m_bIsHost)
    {
        CPacketHandler::GangWarLifecycleEvent__Trigger();
    }

    ProcessAimSync(localPlayer, tickCount);
}

void CSyncEmitCoordinator::ProcessConnectedFrame() {
    const uint32_t stageStart = GetTickCount();

    ProcessMissionSyncState();
    ProcessMissionFlagSync();

    LocalPlayerContext localContext{};
    if (TryGetLocalPlayerContext(localContext))
    {
        ProcessLocalSyncEmit(localContext.player, localContext.tickCount);
    }

    if (GetAsyncKeyState(VK_F7) && GetAsyncKeyState(VK_F10) && GetAsyncKeyState(VK_NUMPAD1))
    {
        *(uint*)0x0 = 1212;
    }

    CFrameStageProfiler::Get().Record(CFrameStageProfiler::Stage::SyncEmit, GetTickCount() - stageStart);
}

void CRemoteEntityProcessingCoordinator::Process() {
    const uint32_t stageStart = GetTickCount();

    CNetworkPedManager::Process();
    CNetworkVehicleManager::ProcessRemoteVehicles();
    CNetworkPlayerManager::ProcessRemotePlayers();
    CNetworkPickupManager::Process();

    CFrameStageProfiler::Get().Record(CFrameStageProfiler::Stage::RemoteEntityProcessing, GetTickCount() - stageStart);
}

void CHudDebugDrawCoordinator::Process() {
    const uint32_t stageStart = GetTickCount();

    if (CEntryExitMarkerSync::ms_bNeedToUpdateAfterProcessingThisFrame
        && CLocalPlayer::m_bIsHost
        && CEntryExitMarkerSync::ms_nLastUpdate + 5000 < GetTickCount())
    {
        CEntryExitMarkerSync::Send();
        CEntryExitMarkerSync::ms_bNeedToUpdateAfterProcessingThisFrame = false;
        CEntryExitMarkerSync::ms_nLastUpdate = GetTickCount();
    }

    if (CNetworkStaticBlip::ms_bNeedToSendAfterThisFrame
        && CLocalPlayer::m_bIsHost)
    {
        CNetworkStaticBlip::Send();
        CNetworkStaticBlip::ms_bNeedToSendAfterThisFrame = false;
    }

    if (GetAsyncKeyState(VK_F11) && CLocalPlayer::m_bIsHost)
    {
        CEntryExitMarkerSync::Send();
    }

    CNetworkCheckpoint::Process();
    CNetworkPlayerNameTag::Process();
    CSniperAimMarkerSync::Process();
    CChat::Draw();
    CChat::DrawInput();

    if (FrontEndMenuManager.m_bPrefsShowHud)
    {
        CNetworkPlayerList::Draw();
        CPacketHandler::DrawCutsceneSkipVoteHud();
    }

    if (FrontEndMenuManager.m_bPrefsShowHud && CCore::Version.stage != SEMVER_STAGE_RELEASE)
    {
        CDXFont::Draw(0, RsGlobal.maximumHeight - CDXFont::m_fFontSize,
            std::string("CoopAndreas " + std::string(COOPANDREAS_VERSION)).c_str(),
            D3DCOLOR_ARGB(255, 160, 160, 160));
    }

    if (CNetwork::m_bConnected && GetAsyncKeyState(VK_F10))
    {
        CDebugPedTasks::Draw();
    }

    if (GetAsyncKeyState(VK_F9))
    {
        char buffer[320];
        const int gamePedCount = CPools::ms_pPedPool ? CPools::ms_pPedPool->GetNoOfUsedSpaces() : -1;
        const int gameVehicleCount = CPools::ms_pVehiclePool ? CPools::ms_pVehiclePool->GetNoOfUsedSpaces() : -1;
        const int entryExitCount = CEntryExitManager::mp_poolEntryExits ? CEntryExitManager::mp_poolEntryExits->GetNoOfUsedSpaces() : -1;
        const unsigned int totalReceivedPackets = CNetwork::m_pClient ? CNetwork::m_pClient->totalReceivedPackets : 0;
        const unsigned int totalSentPackets = CNetwork::m_pClient ? CNetwork::m_pClient->totalSentPackets : 0;
        sprintf(buffer, "IsHost=%d | Game/Network: Peds %d/%d | Cars %d/%d | Recv %u %.2f KB/S | Sent %u %.2f KB/S | EnEx %d", CLocalPlayer::m_bIsHost, gamePedCount, CNetworkPedManager::m_pPeds.size(), gameVehicleCount, CNetworkVehicleManager::m_pVehicles.size(), totalReceivedPackets, CNetwork::ms_nBytesReceivedThisSecond / 1024.0f, totalSentPackets, CNetwork::ms_nBytesSentThisSecond / 1024.0f, entryExitCount);
        CDXFont::Draw(100, 10, buffer, D3DCOLOR_ARGB(255, 255, 255, 255));

        sprintf(buffer, "Interp delay=%ums | Snapshots P:%zu Ped:%zu Veh:%zu | Corrections P:%zu Ped:%zu Veh:%zu",
            CNetworkPlayerManager::InterpTuning::interpolationDelayMs,
            CNetworkPlayerManager::m_streamingOverview.playerSnapshotBacklog,
            CNetworkPlayerManager::m_streamingOverview.pedSnapshotBacklog,
            CNetworkPlayerManager::m_streamingOverview.vehicleSnapshotBacklog,
            CNetworkPlayerManager::m_streamingOverview.playerInterpCorrections,
            CNetworkPlayerManager::m_streamingOverview.pedInterpCorrections,
            CNetworkPlayerManager::m_streamingOverview.vehicleInterpCorrections);
        CDXFont::Draw(100, 28, buffer, D3DCOLOR_ARGB(255, 220, 220, 100));

        int line = 46;
        for (size_t i = 0; i < static_cast<size_t>(CFrameStageProfiler::Stage::Count); ++i)
        {
            auto stage = static_cast<CFrameStageProfiler::Stage>(i);
            const auto& stats = CFrameStageProfiler::Get().GetStats(stage);
            sprintf(buffer, "Stage[%s] last=%ums avg=%ums max=%ums samples=%u",
                StageName(stage),
                stats.lastDurationMs,
                stats.AverageDurationMs(),
                stats.maxDurationMs,
                stats.samples);
            CDXFont::Draw(100, line, buffer, D3DCOLOR_ARGB(255, 130, 220, 255));
            line += 18;
        }

#if DEBUG
        for (size_t i = 0; i < static_cast<size_t>(TickSkipReason::Count); ++i)
        {
            const auto reason = static_cast<TickSkipReason>(i);
            sprintf(buffer, "TickSkip[%s]=%llu", TickSkipReasonName(reason), g_tickSkipCounters[i]);
            CDXFont::Draw(100, line, buffer, D3DCOLOR_ARGB(255, 255, 180, 120));
            line += 18;
        }
#endif
    }

    CFrameStageProfiler::Get().Record(CFrameStageProfiler::Stage::HudDebugDraw, GetTickCount() - stageStart);
}
