#include "stdafx.h"
#include "CMissionSyncState.h"

#include "CCutsceneMgr.h"
#include "CNetwork.h"
#include "ObjectiveSyncState.h"
#include "COpCodeSync.h"
#include <vector>

namespace
{
    using OpcodePreExecuteHandler = bool (*)();

    struct OpCodeHandlerEntry
    {
        uint16_t opcode;
        OpcodePreExecuteHandler handler;
    };

    bool ms_bLoadingCutscene = true;
    bool ms_abLoadingMissionAudio[4] = {};
    bool ms_bProcessingTaskSequence = false;
    bool ms_bMissionActive = false;
    bool ms_bCutsceneActive = false;
    uint32_t ms_lastAuthoritativeCutsceneSkipToken = 0;
    uint32_t ms_pendingConsensusCutsceneSkipToken = 0;

    bool ms_bScriptWideScreenForced = false;
    bool ms_bScriptWideScreenValue = false;
    bool ms_bUserWideScreenPreference = false;
    bool ms_bHasUserWideScreenPreference = false;

    uint32_t ms_nWidescreenEventId = 1;
    uint32_t ms_nLastAppliedWidescreenEventId = 0;
    bool ms_bLastAppliedWidescreenValue = false;
    uint32_t ms_nMissionFlowSequence = 0;
    uint32_t ms_nLastReceivedMissionFlowSequence = 0;
    uint32_t ms_nLastReceivedMissionEventSequence = 0;
    uint32_t ms_missionEpoch = 0;
    uint32_t ms_lastAppliedTerminalRuntimeSessionToken = 0;
    uint8_t ms_lastAppliedTerminalReasonCode = CPackets::MISSION_TERMINAL_REASON_NONE;
    uint16_t ms_runtimeMissionInstanceId = 0;
    uint32_t ms_submissionStateVersion = 0;
    uint32_t ms_submissionSnapshotVersion = 0;
    bool ms_submissionSnapshotInProgress = false;
    bool ms_runtimeSnapshotInProgress = false;
    bool ms_runtimeSnapshotHydrated = false;
    uint32_t ms_runtimeSnapshotVersion = 0;
    uint8_t ms_runtimeSnapshotExpectedActorCount = 0;
    std::vector<CPackets::MissionRuntimeSnapshotActor> ms_runtimeSnapshotActors{};
    CPackets::MissionRuntimeSnapshotState ms_runtimeSnapshotState{};
    // Shared side-content replication state (schools/races/courier/etc).
    //
    // Payload contract:
    // - missionId: start/launch identity (Mission.LoadAndLaunchInternal, race starts that launch mission scripts).
    // - objective: current objective/help text key.
    // - checkpointIndex: monotonically increasing progression index (checkpoint create/update + scripted objective bumps).
    // - timerVisible/timerFrozen/timerMs: HUD timer parity for timer-driven side content.
    // - passFailPending: deterministic terminal outcome latch (1 pass / 2 fail) to avoid duplicate outcomes.
    // - playerControlState: replicated player-control gate for scripted start/end transitions.
    // - movementLocked/aimingLocked/firingLocked/cameraLocked/hudHidden: mission-scoped control constraints.
    //
    // Server stores and replays the latest MissionFlowSync payload so reconnecting and late-join peers
    // immediately hydrate this state without waiting for another opcode edge.
    using SideContentAttemptState = ObjectiveSync::State;
    SideContentAttemptState ms_sideContentAttemptState{};
    struct MissionRuntimePolicy
    {
        uint8_t respawnEligible = 0;
        uint8_t missionFailThreshold = 1;
        uint8_t incapacitationFailThreshold = UINT8_MAX;
    };

    const std::unordered_map<uint16_t, MissionRuntimePolicy> kMissionPolicyOverrides = {
        // Mission scripts with custom fail handling can be added here.
        // { missionId, { respawnEligible, missionFailThreshold, incapacitationFailThreshold } }
    };

    MissionRuntimePolicy ResolveMissionRuntimePolicy(uint16_t missionId)
    {
        auto overrideIt = kMissionPolicyOverrides.find(missionId);
        if (overrideIt != kMissionPolicyOverrides.end())
        {
            return overrideIt->second;
        }

        return MissionRuntimePolicy{};
    }

    bool TrySetTerminalOutcome(uint8_t outcome)
    {
        if (outcome != 1 && outcome != 2)
        {
            return false;
        }

        if (ms_sideContentAttemptState.passFailPending != 0)
        {
            return false;
        }

        const int terminalParams[] = { outcome == 1 ? 1 : 0 };
        return ObjectiveSync::ApplyOpcode(ms_sideContentAttemptState, outcome == 1 ? 0x0318 : 0x045C, terminalParams, 1, nullptr).changed;
    }

    struct SubmissionState
    {
        uint8_t submissionType = 0;
        uint8_t active = 0;
        uint8_t level = 0;
        uint8_t stage = 0;
        uint16_t progress = 0;
        int32_t timerMs = 0;
        int32_t score = 0;
        int32_t rewardCash = 0;
        uint8_t outcome = 0;
        uint8_t participantCount = 0;
        uint8_t currArea = 0;
        CPackets::ReplicatedCheckpointState checkpointState{};
        CPackets::ReplicatedTimerState timerState{};
        uint64_t stateTimestampMs = 0;
        uint32_t stateVersion = 0;
        int32_t lastBroadcastSecond = -1;
    };

    std::unordered_map<uint8_t, SubmissionState> ms_submissionStates{};
    CPackets::ReplicatedCheckpointState ms_authoritativeCheckpointState{};
    CPackets::ReplicatedTimerState ms_authoritativeTimerState{};
    bool ms_hasAuthoritativeFlowState = false;

    constexpr int32_t kTimerDriftSnapThresholdMs = 350;
    constexpr int32_t kTimerDriftSmoothStepMs = 45;
    constexpr uint16_t kCheckpointDriftSnapThreshold = 1;

    constexpr uint16_t OP_END_SCENE_SKIP = 0x0701;
    constexpr uint16_t OP_START_CUTSCENE = 0x02E7;
    constexpr uint16_t OP_CLEAR_CUTSCENE = 0x02EA;
    constexpr uint16_t OP_SWITCH_WIDESCREEN = 0x02A3;
    constexpr uint16_t OP_PRINT_BIG = 0x00BA;
    constexpr uint16_t OP_PRINT_NOW = 0x00BC;
    constexpr uint16_t OP_PRINT_HELP = 0x03E5;
    constexpr uint16_t OP_REGISTER_MISSION_PASSED = 0x0318;
    constexpr uint16_t OP_FAIL_CURRENT_MISSION = 0x045C;
    constexpr uint16_t OP_SET_PLAYER_FIRE_BUTTON = 0x0881;
    constexpr uint16_t OP_DISPLAY_HUD = 0x0826;
    constexpr uint16_t OP_SET_CAMERA_CONTROL = 0x0E60;

    constexpr uint8_t CUTSCENE_PHASE_NONE = 0;
    constexpr uint8_t CUTSCENE_PHASE_INTRO = 1;
    constexpr uint8_t CUTSCENE_PHASE_SKIP = 2;
    constexpr uint8_t CUTSCENE_PHASE_END = 3;
    uint8_t ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_NONE;

    constexpr OpCodeHandlerEntry kPreExecuteOpCodeHandlers[] = {
        { OP_END_SCENE_SKIP, &CMissionSyncState::HandleEndSceneSkip },
        { OP_START_CUTSCENE, &CMissionSyncState::HandleStartCutscene },
        { OP_CLEAR_CUTSCENE, &CMissionSyncState::HandleClearCutscene }
    };

    void ApplyWideScreen(bool enabled)
    {
        if (enabled)
        {
            TheCamera.m_bWideScreenOn = true;
        }
        else
        {
            TheCamera.SetWideScreenOff();
        }
    }

    void ResetScriptWideScreenOverride()
    {
        if (ms_bScriptWideScreenForced && ms_bHasUserWideScreenPreference)
        {
            ApplyWideScreen(ms_bUserWideScreenPreference);
        }

        ms_bScriptWideScreenForced = false;
        ms_bScriptWideScreenValue = false;
        ms_nLastAppliedWidescreenEventId = 0;
    }

    bool IsMissionContextActive()
    {
        return ms_bMissionActive || ms_bCutsceneActive;
    }

    bool IsAuthoritativeObjectiveRenderOpcode(uint16_t opcode)
    {
        return opcode == OP_PRINT_BIG
            || opcode == OP_PRINT_NOW
            || opcode == OP_PRINT_HELP
            || opcode == OP_REGISTER_MISSION_PASSED
            || opcode == OP_FAIL_CURRENT_MISSION;
    }

    struct SubmissionModeAdapter
    {
        uint8_t submissionType;
        std::initializer_list<int> vehicleModels;
    };

    constexpr SubmissionModeAdapter kSubmissionAdapters[] = {
        { CPackets::SUBMISSION_MISSION_TAXI, { MODEL_TAXI, MODEL_CABBIE } },
        { CPackets::SUBMISSION_MISSION_FIREFIGHTER, { MODEL_FIRETRUK } },
        { CPackets::SUBMISSION_MISSION_VIGILANTE, { MODEL_COPCARLA, MODEL_COPCARRU, MODEL_COPCARSF, MODEL_COPBIKE, MODEL_FBIRANCH, MODEL_ENFORCER, MODEL_RHINO } },
        { CPackets::SUBMISSION_MISSION_PARAMEDIC, { MODEL_AMBULAN } },
        { CPackets::SUBMISSION_MISSION_PIMP, { MODEL_BROADWAY } },
        { CPackets::SUBMISSION_MISSION_FREIGHT_TRAIN, { MODEL_FREIGHT, MODEL_STREAK } }
    };

    uint8_t ResolveSubmissionMissionTypeByVehicleModel(int modelId)
    {
        for (const auto& adapter : kSubmissionAdapters)
        {
            for (int allowedModel : adapter.vehicleModels)
            {
                if (allowedModel == modelId)
                {
                    return adapter.submissionType;
                }
            }
        }

        return CPackets::SUBMISSION_MISSION_TYPE_MAX;
    }

    uint8_t CountVehicleParticipants(CVehicle* vehicle)
    {
        if (!vehicle)
        {
            return 0;
        }

        uint8_t participants = vehicle->m_pDriver ? 1 : 0;
        for (int i = 0; i < vehicle->m_nMaxPassengers; ++i)
        {
            if (vehicle->m_apPassengers[i])
            {
                participants++;
            }
        }
        return participants;
    }

    void EmitSubmissionMissionDelta(uint8_t submissionType, uint8_t action, const SubmissionState& state)
    {
        CPackets::SubmissionMissionStateDelta packet{};
        packet.submissionType = submissionType;
        packet.action = action;
        packet.active = state.active;
        packet.level = state.level;
        packet.stage = state.stage;
        packet.progress = state.progress;
        packet.timerMs = state.timerMs;
        packet.score = state.score;
        packet.rewardCash = state.rewardCash;
        packet.outcome = state.outcome;
        packet.participantCount = state.participantCount;
        packet.currArea = static_cast<uint8_t>(CGame::currArea);
        packet.checkpointState = state.checkpointState;
        packet.timerState = state.timerState;
        packet.stateTimestampMs = GetTickCount64();
        packet.stateVersion = ++ms_submissionStateVersion;

        CNetwork::SendPacket(CPacketsID::SUBMISSION_MISSION_STATE_DELTA, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }

    void ApplyReplicatedControlLocks(const SideContentAttemptState& state)
    {
        CPad* pad = CPad::GetPad(0);
        if (!pad)
        {
            return;
        }

        const bool movementLocked = state.movementLocked != 0;
        const bool aimingLocked = state.aimingLocked != 0;
        const bool firingLocked = state.firingLocked != 0;
        const bool hudHidden = state.hudHidden != 0;

        pad->bDisablePlayerEnterCar = movementLocked ? 1 : 0;
        pad->bDisablePlayerDuck = movementLocked ? 1 : 0;
        pad->bDisablePlayerCycleWeapon = movementLocked ? 1 : 0;
        pad->bDisablePlayerJump = movementLocked ? 1 : 0;
        pad->bDisablePlayerFireWeapon = (firingLocked || aimingLocked) ? 1 : 0;
        pad->bDisablePlayerFireWeaponWithL1 = (firingLocked || aimingLocked) ? 1 : 0;
        pad->bDisablePlayerDisplayVitalStats = hudHidden ? 1 : 0;
    }

    void ApplyMissionCleanupFromTerminal()
    {
        CNetworkCheckpoint::Remove();
        CNetworkEntityBlip::ClearEntityBlips();
        ms_sideContentAttemptState.playerControlState = 1;
        ms_sideContentAttemptState.movementLocked = 0;
        ms_sideContentAttemptState.aimingLocked = 0;
        ms_sideContentAttemptState.firingLocked = 0;
        ms_sideContentAttemptState.cameraLocked = 0;
        ms_sideContentAttemptState.hudHidden = 0;

        CPad* pad = CPad::GetPad(0);
        if (pad)
        {
            pad->SetDrunkInputDelay(0);
            pad->bApplyBrakes = 0;
            pad->bDisablePlayerEnterCar = 0;
            pad->bDisablePlayerDuck = 0;
            pad->bDisablePlayerFireWeapon = 0;
            pad->bDisablePlayerFireWeaponWithL1 = 0;
            pad->bDisablePlayerCycleWeapon = 0;
            pad->bDisablePlayerJump = 0;
            pad->bDisablePlayerDisplayVitalStats = 0;
        }

        CHud::m_BigMessage[0][0] = 0;
        CHud::m_BigMessage[1][0] = 0;
        CDraw::FadeValue = 0;
    }

    bool ShouldSnapCheckpointState(const SideContentAttemptState& state, const CPackets::ReplicatedCheckpointState& target)
    {
        const uint16_t currentIndex = state.checkpointIndex;
        const uint16_t currentCount = state.checkpointCount;
        const uint8_t currentActive = currentCount > 0 ? 1 : 0;

        return static_cast<uint16_t>(std::abs(static_cast<int>(currentIndex) - static_cast<int>(target.currentIndex))) > kCheckpointDriftSnapThreshold
            || currentCount != target.totalCount
            || currentActive != target.activeFlag;
    }

    int32_t ResolveAuthoritativeTimerRemainingMs(uint32_t nowTick)
    {
        if (ms_authoritativeTimerState.paused != 0)
        {
            return std::max(ms_authoritativeTimerState.remaining, 0);
        }

        const uint32_t elapsed = nowTick - ms_authoritativeTimerState.startTick;
        return std::max(ms_authoritativeTimerState.remaining - static_cast<int32_t>(elapsed), 0);
    }

    void ProcessFlowStateDriftCorrection()
    {
        if (CLocalPlayer::m_bIsHost || !ms_hasAuthoritativeFlowState)
        {
            return;
        }

        const uint32_t nowTick = GetTickCount();
        const int32_t authoritativeRemaining = ResolveAuthoritativeTimerRemainingMs(nowTick);
        const int32_t timerDelta = authoritativeRemaining - ms_sideContentAttemptState.timerMs;

        if (std::abs(timerDelta) >= kTimerDriftSnapThresholdMs)
        {
            ms_sideContentAttemptState.timerMs += std::clamp(timerDelta, -kTimerDriftSmoothStepMs, kTimerDriftSmoothStepMs);
        }
        else
        {
            ms_sideContentAttemptState.timerMs = authoritativeRemaining;
        }

        const int checkpointDelta = static_cast<int>(ms_authoritativeCheckpointState.currentIndex) - static_cast<int>(ms_sideContentAttemptState.checkpointIndex);
        if (checkpointDelta != 0)
        {
            ms_sideContentAttemptState.checkpointIndex = static_cast<uint16_t>(
                static_cast<int>(ms_sideContentAttemptState.checkpointIndex) + (checkpointDelta > 0 ? 1 : -1));
        }

        if (ms_sideContentAttemptState.checkpointCount != ms_authoritativeCheckpointState.totalCount)
        {
            const int checkpointCountDelta =
                static_cast<int>(ms_authoritativeCheckpointState.totalCount) - static_cast<int>(ms_sideContentAttemptState.checkpointCount);
            ms_sideContentAttemptState.checkpointCount = static_cast<uint16_t>(
                static_cast<int>(ms_sideContentAttemptState.checkpointCount) + (checkpointCountDelta > 0 ? 1 : -1));
        }
    }

    void PopulateMissionFlowStatePayload(CPackets::MissionFlowSync& packet, const SideContentAttemptState& state)
    {
        packet.missionId = state.missionId;
        packet.timerMs = state.timerMs;
        packet.objectivePhaseIndex = state.objectivePhaseIndex;
        packet.checkpointIndex = state.checkpointIndex;
        packet.checkpointCount = state.checkpointCount;
        packet.timerVisible = state.timerVisible;
        packet.timerFrozen = state.timerFrozen;
        packet.timerDirection = state.timerDirection;
        packet.passFailPending = state.passFailPending;
        packet.playerControlState = state.playerControlState;
        packet.movementLocked = state.movementLocked;
        packet.aimingLocked = state.aimingLocked;
        packet.firingLocked = state.firingLocked;
        packet.cameraLocked = state.cameraLocked;
        packet.hudHidden = state.hudHidden;
        packet.cutscenePhase = state.cutscenePhase;
        packet.cutsceneSessionToken = state.cutsceneSessionToken;
        packet.objectiveTextToken = state.objectiveTextToken;
        packet.objectiveTextSemantics = state.objectiveTextSemantics;
        strncpy_s(packet.objective, state.objective, sizeof(packet.objective));
        packet.respawnEligible = state.respawnEligible;
        packet.participantDeathCount = state.participantDeathCount;
        packet.participantIncapacitationCount = state.participantIncapacitationCount;
        packet.respawnCount = state.respawnCount;
        packet.missionFailThreshold = state.missionFailThreshold;
        packet.incapacitationFailThreshold = state.incapacitationFailThreshold;
        packet.vehicleTaskState = state.vehicleTaskState;
        packet.pursuitState = state.pursuitState;
        packet.destroyEscapeState = state.destroyEscapeState;
        packet.vehicleTaskSequence = state.vehicleTaskSequence;
        packet.terminalTieBreaker =
            state.passFailPending == 2 ? 3 :
            (state.passFailPending == 1 ? 2 : 0);
    }

    void ApplyInboundObjectiveTextState(SideContentAttemptState& state, const CPackets::MissionFlowSync& packet)
    {
        state.objectiveTextToken = packet.objectiveTextToken;
        state.objectiveTextSemantics = packet.objectiveTextSemantics;

        if (packet.objectiveTextSemantics == static_cast<uint8_t>(ObjectiveSync::ObjectiveTextSemantics::Clear))
        {
            memset(state.objective, 0, sizeof(state.objective));
            return;
        }

        if (packet.objectiveTextSemantics == static_cast<uint8_t>(ObjectiveSync::ObjectiveTextSemantics::Replace))
        {
            strncpy_s(state.objective, packet.objective, sizeof(state.objective));
            return;
        }

        if (packet.objective[0])
        {
            strncpy_s(state.objective, packet.objective, sizeof(state.objective));
        }
        else
        {
            memset(state.objective, 0, sizeof(state.objective));
        }
    }
}

void CMissionSyncState::Init()
{
    ms_bLoadingCutscene = true;
    memset(ms_abLoadingMissionAudio, 0, sizeof(ms_abLoadingMissionAudio));
    ms_bProcessingTaskSequence = false;
    ms_bMissionActive = false;
    ms_bCutsceneActive = false;
    ms_lastAuthoritativeCutsceneSkipToken = 0;
    ms_pendingConsensusCutsceneSkipToken = 0;
    ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_NONE;

    ms_bScriptWideScreenForced = false;
    ms_bScriptWideScreenValue = false;
    ms_bUserWideScreenPreference = false;
    ms_bHasUserWideScreenPreference = false;

    ms_nWidescreenEventId = 1;
    ms_nLastAppliedWidescreenEventId = 0;
    ms_bLastAppliedWidescreenValue = false;
    ms_nMissionFlowSequence = 0;
    ms_nLastReceivedMissionFlowSequence = 0;
    ms_nLastReceivedMissionEventSequence = 0;
    ms_missionEpoch = 0;
    ms_lastAppliedTerminalRuntimeSessionToken = 0;
    ms_lastAppliedTerminalReasonCode = CPackets::MISSION_TERMINAL_REASON_NONE;
    ms_runtimeMissionInstanceId = 0;
    ms_submissionStateVersion = 0;
    ms_submissionSnapshotVersion = 0;
    ms_submissionSnapshotInProgress = false;
    ms_runtimeSnapshotInProgress = false;
    ms_runtimeSnapshotHydrated = false;
    ms_runtimeSnapshotVersion = 0;
    ms_runtimeSnapshotExpectedActorCount = 0;
    ms_runtimeSnapshotActors.clear();
    ms_runtimeSnapshotState = {};
    ms_hasAuthoritativeFlowState = false;
    ms_authoritativeCheckpointState = {};
    ms_authoritativeTimerState = {};
    ms_submissionStates.clear();
    ms_sideContentAttemptState = {};
}

bool CMissionSyncState::CanAcceptLiveMissionEvents()
{
    return !ms_runtimeSnapshotInProgress;
}

bool CMissionSyncState::IsProcessingTaskSequence()
{
    return ms_bProcessingTaskSequence;
}

void CMissionSyncState::SetProcessingTaskSequence(bool processing)
{
    ms_bProcessingTaskSequence = processing;
}

void CMissionSyncState::MarkMissionAudioLoading(uint8_t slotId)
{
    if (slotId < ARRAY_SIZE(ms_abLoadingMissionAudio))
    {
        ms_abLoadingMissionAudio[slotId] = true;
    }
}

void CMissionSyncState::ProcessMissionAudioLoading()
{
    if (CLocalPlayer::m_bIsHost)
    {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(ms_abLoadingMissionAudio); i++)
    {
        if (ms_abLoadingMissionAudio[i]
            && plugin::CallMethodAndReturn<int8_t, 0x5072A0>(&AudioEngine, i) == 1)
        {
            plugin::CallMethod<0x5072B0>(&AudioEngine, i);
            ms_abLoadingMissionAudio[i] = false;
        }
    }
}

void CMissionSyncState::ProcessDeferredCutsceneStart()
{
    if (!ms_bLoadingCutscene
        || CLocalPlayer::m_bIsHost
        || !CCutsceneMgr::ms_cutsceneName[0]
        || CCutsceneMgr::ms_cutsceneLoadStatus != 2)
    {
        return;
    }

    ms_bLoadingCutscene = false;
    Command<Commands::START_CUTSCENE>();
}

void CMissionSyncState::ProcessWidescreenPolicy()
{
    if (CLocalPlayer::m_bIsHost)
    {
        return;
    }

    if (!ms_bScriptWideScreenForced)
    {
        ms_bUserWideScreenPreference = TheCamera.m_bWideScreenOn;
        ms_bHasUserWideScreenPreference = true;
        return;
    }

    if (TheCamera.m_bWideScreenOn != ms_bScriptWideScreenValue)
    {
        ApplyWideScreen(ms_bScriptWideScreenValue);
    }
}

void CMissionSyncState::HandleMissionFlagSync(bool onMission)
{
    const bool wasMissionActive = ms_bMissionActive;
    ms_bMissionActive = onMission;

    if (wasMissionActive == onMission)
    {
        return;
    }

    ms_nWidescreenEventId++;
    if (onMission)
    {
        ms_runtimeMissionInstanceId++;
        if (ms_runtimeMissionInstanceId == 0)
        {
            ms_runtimeMissionInstanceId = 1;
        }
    }

    if (!onMission)
    {
        ResetScriptWideScreenOverride();
        ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_NONE;
        ms_pendingConsensusCutsceneSkipToken = 0;
    }
}

uint16_t CMissionSyncState::GetMissionInstanceId()
{
    if (ms_sideContentAttemptState.missionId != 0)
    {
        return ms_sideContentAttemptState.missionId;
    }

    return ms_runtimeMissionInstanceId;
}

void CMissionSyncState::EmitMissionFlowCutscenePhase(uint8_t phase, const char* cutsceneName, uint8_t currArea, uint32_t cutsceneSessionToken)
{
    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    CPackets::MissionFlowSync packet{};
    if (phase == CUTSCENE_PHASE_SKIP)
    {
        packet.eventType = CPackets::MISSION_FLOW_EVENT_CUTSCENE_SKIP;
    }
    else if (phase == CUTSCENE_PHASE_END)
    {
        packet.eventType = CPackets::MISSION_FLOW_EVENT_CUTSCENE_END;
    }
    else
    {
        packet.eventType = CPackets::MISSION_FLOW_EVENT_CUTSCENE_INTRO;
    }
    packet.currArea = currArea;
    packet.onMission = (CTheScripts::OnAMissionFlag && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag]) ? 1 : 0;
    packet.sequence = NextMissionEventSequenceId();
    packet.sequenceId = packet.sequence;
    packet.missionEpoch = ms_missionEpoch;
    packet.cutscenePhase = phase;
    packet.cutsceneSessionToken = cutsceneSessionToken;
    PopulateMissionFlowStatePayload(packet, ms_sideContentAttemptState);
    packet.cutscenePhase = phase;
    packet.cutsceneSessionToken = cutsceneSessionToken;
    strncpy_s(packet.cutsceneName, cutsceneName ? cutsceneName : "", sizeof(packet.cutsceneName));
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::EmitMissionFlowText(uint16_t opcode, const HostTextMessage& message)
{
    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    CPackets::MissionFlowSync packet{};
    if (opcode == 0x0318)
    {
        if (!TrySetTerminalOutcome(1))
        {
            return;
        }
        packet.eventType = CPackets::MISSION_FLOW_EVENT_PASS;
    }
    else if (opcode == 0x045C)
    {
        if (!TrySetTerminalOutcome(2))
        {
            return;
        }
        packet.eventType = CPackets::MISSION_FLOW_EVENT_FAIL;
    }
    else
    {
        packet.eventType = CPackets::MISSION_FLOW_EVENT_OBJECTIVE;
    }

    packet.messageType = message.messageType;
    packet.time = message.time;
    packet.flag = message.flag;
    packet.currArea = (uint8_t)CGame::currArea;
    packet.onMission = (CTheScripts::OnAMissionFlag && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag]) ? 1 : 0;
    packet.sequence = NextMissionEventSequenceId();
    packet.sequenceId = packet.sequence;
    packet.missionEpoch = ms_missionEpoch;
    packet.sourceOpcode = opcode;
    const ObjectiveSync::ObjectiveTextSemantics semantics =
        (message.gxt && message.gxt[0]) ? ObjectiveSync::ObjectiveTextSemantics::Replace : ObjectiveSync::ObjectiveTextSemantics::Clear;
    if (ObjectiveSync::ApplyObjectiveTextEvent(ms_sideContentAttemptState, semantics, message.gxt))
    {
        if (ms_sideContentAttemptState.checkpointIndex < UINT16_MAX)
        {
            ++ms_sideContentAttemptState.checkpointIndex;
        }
    }

    PopulateMissionFlowStatePayload(packet, ms_sideContentAttemptState);
    strncpy_s(packet.gxt, message.gxt ? message.gxt : "", sizeof(packet.gxt));
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::EmitMissionFlowOpcode(uint16_t opcode, const int* params, uint16_t paramCount, const char* text)
{
    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    const ObjectiveSync::ApplyResult applyResult = ObjectiveSync::ApplyOpcode(ms_sideContentAttemptState, opcode, params, paramCount, text);
    if (!applyResult.changed)
    {
        return;
    }

    CPackets::MissionFlowSync packet{};
    packet.eventType = CPackets::MISSION_FLOW_EVENT_STATE_UPDATE;
    packet.currArea = (uint8_t)CGame::currArea;
    packet.onMission = (CTheScripts::OnAMissionFlag && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag]) ? 1 : 0;
    packet.sequence = NextMissionEventSequenceId();
    packet.sequenceId = packet.sequence;
    packet.missionEpoch = ms_missionEpoch;
    packet.sourceOpcode = opcode;
    PopulateMissionFlowStatePayload(packet, ms_sideContentAttemptState);
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::EmitParticipantRuntimeEvent(uint8_t eventType, bool respawnEligible)
{
    if (!CLocalPlayer::m_bIsHost || !IsMissionContextActive())
    {
        return;
    }

    if (ms_sideContentAttemptState.missionFailThreshold == 0)
    {
        const MissionRuntimePolicy policy = ResolveMissionRuntimePolicy(ms_sideContentAttemptState.missionId);
        ms_sideContentAttemptState.missionFailThreshold = policy.missionFailThreshold;
        ms_sideContentAttemptState.incapacitationFailThreshold = policy.incapacitationFailThreshold;
        ms_sideContentAttemptState.respawnEligible = policy.respawnEligible;
    }

    if (eventType == CPackets::MISSION_FLOW_EVENT_PARTICIPANT_DEATH)
    {
        ms_sideContentAttemptState.participantDeathCount = static_cast<uint8_t>(std::min<int>(ms_sideContentAttemptState.participantDeathCount + 1, UINT8_MAX));
    }
    else if (eventType == CPackets::MISSION_FLOW_EVENT_PARTICIPANT_INCAPACITATED)
    {
        ms_sideContentAttemptState.participantIncapacitationCount = static_cast<uint8_t>(std::min<int>(ms_sideContentAttemptState.participantIncapacitationCount + 1, UINT8_MAX));
    }
    else if (eventType == CPackets::MISSION_FLOW_EVENT_PARTICIPANT_RESPAWNED)
    {
        ms_sideContentAttemptState.respawnCount = static_cast<uint8_t>(std::min<int>(ms_sideContentAttemptState.respawnCount + 1, UINT8_MAX));
        ms_sideContentAttemptState.respawnEligible = 0;
        if (ms_sideContentAttemptState.checkpointIndex < UINT16_MAX)
        {
            ++ms_sideContentAttemptState.checkpointIndex;
        }
    }
    else if (eventType == CPackets::MISSION_FLOW_EVENT_RESPAWN_ELIGIBILITY)
    {
        ms_sideContentAttemptState.respawnEligible = respawnEligible ? 1 : 0;
    }

    if (ms_sideContentAttemptState.passFailPending == 0)
    {
        if (ms_sideContentAttemptState.missionFailThreshold > 0
            && ms_sideContentAttemptState.participantDeathCount >= ms_sideContentAttemptState.missionFailThreshold)
        {
            TrySetTerminalOutcome(2);
        }
        if (ms_sideContentAttemptState.incapacitationFailThreshold != UINT8_MAX
            && ms_sideContentAttemptState.participantIncapacitationCount >= ms_sideContentAttemptState.incapacitationFailThreshold)
        {
            TrySetTerminalOutcome(2);
        }
    }

    CPackets::MissionFlowSync packet{};
    packet.eventType = eventType;
    packet.currArea = static_cast<uint8_t>(CGame::currArea);
    packet.onMission = (CTheScripts::OnAMissionFlag && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag]) ? 1 : 0;
    packet.sequence = NextMissionEventSequenceId();
    packet.sequenceId = packet.sequence;
    packet.missionEpoch = ms_missionEpoch;
    PopulateMissionFlowStatePayload(packet, ms_sideContentAttemptState);
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::HandleMissionFlowSync(const CPackets::MissionFlowSync& packet)
{
    const bool isHostReplay = CLocalPlayer::m_bIsHost && packet.replay != 0;
    if (!ShouldAcceptInboundMissionEvent(packet.missionEpoch, packet.sequenceId))
    {
        return;
    }

    if (CLocalPlayer::m_bIsHost && !isHostReplay)
    {
        return;
    }

    if (packet.sequence != 0 && packet.sequence <= ms_nLastReceivedMissionFlowSequence)
    {
        return;
    }

    ms_nLastReceivedMissionFlowSequence = packet.sequence;
    if (!CLocalPlayer::m_bIsHost)
    {
        HandleMissionFlagSync(packet.onMission != 0);
    }

    const bool newAttempt = packet.sourceOpcode == 0x0417
        || (packet.missionId != 0 && packet.missionId != ms_sideContentAttemptState.missionId);
    if (newAttempt)
    {
        ms_sideContentAttemptState = {};
        const MissionRuntimePolicy policy = ResolveMissionRuntimePolicy(packet.missionId);
        ms_sideContentAttemptState.respawnEligible = policy.respawnEligible;
        ms_sideContentAttemptState.missionFailThreshold = policy.missionFailThreshold;
        ms_sideContentAttemptState.incapacitationFailThreshold = policy.incapacitationFailThreshold;
        ms_hasAuthoritativeFlowState = false;
        ms_authoritativeCheckpointState = {};
        ms_authoritativeTimerState = {};
    }

    ms_sideContentAttemptState.missionId = packet.missionId;
    ms_sideContentAttemptState.objectivePhaseIndex = packet.objectivePhaseIndex;
    ms_sideContentAttemptState.timerVisible = packet.timerVisible;
    ms_sideContentAttemptState.timerFrozen = packet.timerFrozen;
    ms_sideContentAttemptState.timerDirection = packet.timerDirection;
    ms_sideContentAttemptState.playerControlState = packet.playerControlState;
    ms_sideContentAttemptState.movementLocked = packet.movementLocked;
    ms_sideContentAttemptState.aimingLocked = packet.aimingLocked;
    ms_sideContentAttemptState.firingLocked = packet.firingLocked;
    ms_sideContentAttemptState.cameraLocked = packet.cameraLocked;
    ms_sideContentAttemptState.hudHidden = packet.hudHidden;
    ms_sideContentAttemptState.cutscenePhase = packet.cutscenePhase;
    ms_sideContentAttemptState.cutsceneSessionToken = packet.cutsceneSessionToken;
    ms_sideContentAttemptState.respawnEligible = packet.respawnEligible;
    ms_sideContentAttemptState.participantDeathCount = packet.participantDeathCount;
    ms_sideContentAttemptState.participantIncapacitationCount = packet.participantIncapacitationCount;
    ms_sideContentAttemptState.respawnCount = packet.respawnCount;
    ms_sideContentAttemptState.missionFailThreshold = packet.missionFailThreshold == 0 ? 1 : packet.missionFailThreshold;
    ms_sideContentAttemptState.incapacitationFailThreshold = packet.incapacitationFailThreshold;
    ms_sideContentAttemptState.vehicleTaskState = packet.vehicleTaskState;
    ms_sideContentAttemptState.pursuitState = packet.pursuitState;
    ms_sideContentAttemptState.destroyEscapeState = packet.destroyEscapeState;
    ms_sideContentAttemptState.vehicleTaskSequence = packet.vehicleTaskSequence;
    ApplyInboundObjectiveTextState(ms_sideContentAttemptState, packet);

    CPackets::ReplicatedCheckpointState authoritativeCheckpoint{};
    authoritativeCheckpoint.currentIndex = packet.checkpointIndex;
    authoritativeCheckpoint.totalCount = packet.checkpointCount;
    authoritativeCheckpoint.activeFlag = packet.checkpointCount > 0 ? 1 : 0;

    CPackets::ReplicatedTimerState authoritativeTimer{};
    authoritativeTimer.startTick = GetTickCount();
    authoritativeTimer.remaining = std::max(packet.timerMs, 0);
    authoritativeTimer.paused = packet.timerFrozen ? 1 : 0;

    const bool shouldBlendCheckpoint = ms_hasAuthoritativeFlowState
        && ShouldSnapCheckpointState(ms_sideContentAttemptState, authoritativeCheckpoint);
    const bool shouldBlendTimer = ms_hasAuthoritativeFlowState
        && std::abs(ms_sideContentAttemptState.timerMs - authoritativeTimer.remaining) >= kTimerDriftSnapThresholdMs;

    ms_authoritativeCheckpointState = authoritativeCheckpoint;
    ms_authoritativeTimerState = authoritativeTimer;
    ms_hasAuthoritativeFlowState = true;

    if (!shouldBlendCheckpoint)
    {
        ms_sideContentAttemptState.checkpointIndex = authoritativeCheckpoint.currentIndex;
        ms_sideContentAttemptState.checkpointCount = authoritativeCheckpoint.totalCount;
    }
    if (!shouldBlendTimer)
    {
        ms_sideContentAttemptState.timerMs = authoritativeTimer.remaining;
    }

    if (ms_sideContentAttemptState.passFailPending == 0)
    {
        ms_sideContentAttemptState.passFailPending = packet.passFailPending;
    }

    ApplyAdjudicatedTerminalState(packet);

    if (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_INTRO)
    {
        ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_INTRO;
        ms_bCutsceneActive = true;
    }
    else if (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_SKIP || packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_END)
    {
        ms_pendingReplicatedCutscenePhase =
            (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_END) ? CUTSCENE_PHASE_END : CUTSCENE_PHASE_SKIP;
        if (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_SKIP)
        {
            ms_pendingConsensusCutsceneSkipToken = packet.cutsceneSessionToken != 0 ? packet.cutsceneSessionToken : 1;
        }
        ms_bCutsceneActive = false;
        ResetScriptWideScreenOverride();
    }

    ApplyReplicatedControlLocks(ms_sideContentAttemptState);

    if (!CLocalPlayer::m_bIsHost
        && (packet.eventType == CPackets::MISSION_FLOW_EVENT_OBJECTIVE
            || packet.eventType == CPackets::MISSION_FLOW_EVENT_FAIL
            || packet.eventType == CPackets::MISSION_FLOW_EVENT_PASS))
    {
        if ((packet.eventType == CPackets::MISSION_FLOW_EVENT_FAIL || packet.eventType == CPackets::MISSION_FLOW_EVENT_PASS)
            && packet.passFailPending == 0)
        {
            return;
        }

        char gxt[9];
        strncpy_s(gxt, packet.gxt, 8);
        gxt[8] = '\0';

        switch (packet.messageType)
        {
        case 1: Command<Commands::PRINT_BIG>(gxt, packet.time, packet.flag); break;
        case 2: Command<Commands::PRINT_NOW>(gxt, packet.time, packet.flag); break;
        case 3: Command<Commands::PRINT_HELP>(gxt); break;
        default: Command<Commands::PRINT>(gxt, packet.time, packet.flag); break;
        }
    }
}

void CMissionSyncState::HandleMissionRuntimeSnapshotBegin(const CPackets::MissionRuntimeSnapshotBegin& packet)
{
    ms_runtimeSnapshotInProgress = true;
    ms_runtimeSnapshotHydrated = false;
    ms_runtimeSnapshotVersion = packet.snapshotVersion;
    ms_runtimeSnapshotExpectedActorCount = packet.actorCount;
    ms_runtimeSnapshotActors.clear();
    ms_runtimeSnapshotState = {};
}

void CMissionSyncState::HandleMissionRuntimeSnapshotState(const CPackets::MissionRuntimeSnapshotState& packet)
{
    if (!ms_runtimeSnapshotInProgress)
    {
        return;
    }

    ms_runtimeSnapshotState = packet;
}

void CMissionSyncState::HandleMissionRuntimeSnapshotActor(const CPackets::MissionRuntimeSnapshotActor& packet)
{
    if (!ms_runtimeSnapshotInProgress)
    {
        return;
    }

    if (ms_runtimeSnapshotActors.size() >= UINT8_MAX)
    {
        return;
    }

    ms_runtimeSnapshotActors.push_back(packet);

    if (packet.missionEpoch != 0
        && packet.scriptLocalIdentifier != 0
        && packet.actorNetworkId >= 0)
    {
        COpCodeSync::RegistrySnapshotEntry entry{};
        entry.missionEpoch = packet.missionEpoch;
        entry.scriptLocalIdentifier = packet.scriptLocalIdentifier;
        entry.opcode = packet.sourceOpcode;
        entry.slot = packet.sourceSlot;
        entry.entityType = static_cast<eSyncedParamType>(packet.actorType);
        entry.networkId = packet.actorNetworkId;
        COpCodeSync::ImportRegistrySnapshotEntry(entry);
    }
}

void CMissionSyncState::HandleMissionRuntimeSnapshotEnd(const CPackets::MissionRuntimeSnapshotEnd& packet)
{
    if (!ms_runtimeSnapshotInProgress)
    {
        return;
    }

    if (packet.snapshotVersion != 0 && ms_runtimeSnapshotVersion != 0 && packet.snapshotVersion != ms_runtimeSnapshotVersion)
    {
        ms_runtimeSnapshotInProgress = false;
        return;
    }

    ms_sideContentAttemptState.missionId = ms_runtimeSnapshotState.missionId;
    ms_sideContentAttemptState.objectivePhaseIndex = ms_runtimeSnapshotState.objectivePhaseIndex;
    ms_sideContentAttemptState.timerMs = std::max(ms_runtimeSnapshotState.timerMs, 0);
    ms_sideContentAttemptState.timerVisible = ms_runtimeSnapshotState.timerVisible;
    ms_sideContentAttemptState.timerFrozen = ms_runtimeSnapshotState.timerFrozen;
    ms_sideContentAttemptState.timerDirection = ms_runtimeSnapshotState.timerDirection;
    ms_sideContentAttemptState.checkpointIndex = ms_runtimeSnapshotState.checkpointIndex;
    ms_sideContentAttemptState.checkpointCount = ms_runtimeSnapshotState.checkpointCount;
    ms_sideContentAttemptState.playerControlState = ms_runtimeSnapshotState.playerControlState;
    ms_sideContentAttemptState.movementLocked = ms_runtimeSnapshotState.movementLocked;
    ms_sideContentAttemptState.aimingLocked = ms_runtimeSnapshotState.aimingLocked;
    ms_sideContentAttemptState.firingLocked = ms_runtimeSnapshotState.firingLocked;
    ms_sideContentAttemptState.cameraLocked = ms_runtimeSnapshotState.cameraLocked;
    ms_sideContentAttemptState.hudHidden = ms_runtimeSnapshotState.hudHidden;
    ms_sideContentAttemptState.passFailPending = ms_runtimeSnapshotState.passFailPending;
    ms_sideContentAttemptState.cutscenePhase = ms_runtimeSnapshotState.cutscenePhase;
    ms_sideContentAttemptState.cutsceneSessionToken = ms_runtimeSnapshotState.cutsceneSessionToken;
    ms_sideContentAttemptState.respawnEligible = ms_runtimeSnapshotState.respawnEligible;
    ms_sideContentAttemptState.participantDeathCount = ms_runtimeSnapshotState.participantDeathCount;
    ms_sideContentAttemptState.participantIncapacitationCount = ms_runtimeSnapshotState.participantIncapacitationCount;
    ms_sideContentAttemptState.respawnCount = ms_runtimeSnapshotState.respawnCount;
    ms_sideContentAttemptState.missionFailThreshold = ms_runtimeSnapshotState.missionFailThreshold == 0 ? 1 : ms_runtimeSnapshotState.missionFailThreshold;
    ms_sideContentAttemptState.incapacitationFailThreshold = ms_runtimeSnapshotState.incapacitationFailThreshold;
    ms_sideContentAttemptState.vehicleTaskState = ms_runtimeSnapshotState.vehicleTaskState;
    ms_sideContentAttemptState.pursuitState = ms_runtimeSnapshotState.pursuitState;
    ms_sideContentAttemptState.destroyEscapeState = ms_runtimeSnapshotState.destroyEscapeState;
    ms_sideContentAttemptState.vehicleTaskSequence = ms_runtimeSnapshotState.vehicleTaskSequence;
    ms_sideContentAttemptState.objectiveTextToken = ms_runtimeSnapshotState.objectiveTextToken;
    ms_sideContentAttemptState.objectiveTextSemantics = ms_runtimeSnapshotState.objectiveTextSemantics;
    std::memcpy(ms_sideContentAttemptState.objective, ms_runtimeSnapshotState.objective, sizeof(ms_sideContentAttemptState.objective));

    ms_authoritativeCheckpointState.currentIndex = ms_runtimeSnapshotState.checkpointIndex;
    ms_authoritativeCheckpointState.totalCount = ms_runtimeSnapshotState.checkpointCount;
    ms_authoritativeCheckpointState.activeFlag = ms_runtimeSnapshotState.checkpointCount > 0 ? 1 : 0;
    ms_authoritativeTimerState.startTick = GetTickCount();
    ms_authoritativeTimerState.remaining = std::max(ms_runtimeSnapshotState.timerMs, 0);
    ms_authoritativeTimerState.paused = ms_runtimeSnapshotState.timerFrozen ? 1 : 0;
    ms_hasAuthoritativeFlowState = true;

    SetMissionAuthorityMetadata(ms_runtimeSnapshotState.missionEpoch, ms_runtimeSnapshotState.sequenceId);
    HandleMissionFlagSync(ms_runtimeSnapshotState.onMission != 0);

    CPackets::MissionFlowSync terminalView{};
    terminalView.terminalReasonCode = ms_runtimeSnapshotState.terminalReasonCode;
    terminalView.terminalSourceEventType = ms_runtimeSnapshotState.terminalSourceEventType;
    terminalView.terminalSourceOpcode = ms_runtimeSnapshotState.terminalSourceOpcode;
    terminalView.terminalSourceSequence = ms_runtimeSnapshotState.terminalSourceSequence;
    terminalView.runtimeSessionToken = ms_runtimeSnapshotState.runtimeSessionToken;
    terminalView.passFailPending = ms_runtimeSnapshotState.passFailPending;
    terminalView.terminalTieBreaker = ms_runtimeSnapshotState.terminalTieBreaker;
    ApplyAdjudicatedTerminalState(terminalView);
    ApplyReplicatedControlLocks(ms_sideContentAttemptState);

    ms_runtimeSnapshotHydrated = true;
    ms_runtimeSnapshotInProgress = false;
}

void CMissionSyncState::SetMissionAuthorityEpoch(uint32_t missionEpoch)
{
    SetMissionAuthorityMetadata(missionEpoch, 0);
}

void CMissionSyncState::SetMissionAuthorityMetadata(uint32_t missionEpoch, uint32_t sequenceId)
{
    if (missionEpoch == 0)
    {
        return;
    }

    if (missionEpoch > ms_missionEpoch)
    {
        COpCodeSync::ClearRegistryForEpoch(ms_missionEpoch);
        ms_missionEpoch = missionEpoch;
        ms_nLastReceivedMissionFlowSequence = 0;
        ms_nLastReceivedMissionEventSequence = sequenceId;
        ms_nMissionFlowSequence = sequenceId;
        return;
    }

    if (missionEpoch == ms_missionEpoch && sequenceId > ms_nMissionFlowSequence)
    {
        ms_nMissionFlowSequence = sequenceId;
    }
    if (missionEpoch == ms_missionEpoch && sequenceId > ms_nLastReceivedMissionEventSequence)
    {
        ms_nLastReceivedMissionEventSequence = sequenceId;
    }
}

uint32_t CMissionSyncState::GetMissionAuthorityEpoch()
{
    return ms_missionEpoch;
}

uint32_t CMissionSyncState::NextMissionEventSequenceId()
{
    return ++ms_nMissionFlowSequence;
}

bool CMissionSyncState::ShouldAcceptInboundMissionEvent(uint32_t missionEpoch, uint32_t sequenceId)
{
    if (missionEpoch < ms_missionEpoch)
    {
        return false;
    }

    if (missionEpoch > ms_missionEpoch)
    {
        ms_missionEpoch = missionEpoch;
        ms_nLastReceivedMissionFlowSequence = 0;
        ms_nLastReceivedMissionEventSequence = 0;
        ms_nMissionFlowSequence = 0;
    }

    if (sequenceId != 0 && sequenceId <= ms_nLastReceivedMissionEventSequence)
    {
        return false;
    }

    if (sequenceId > ms_nLastReceivedMissionEventSequence)
    {
        ms_nLastReceivedMissionEventSequence = sequenceId;
    }

    return true;
}

void CMissionSyncState::ApplyAdjudicatedTerminalState(const CPackets::MissionFlowSync& packet)
{
    if (packet.terminalReasonCode == CPackets::MISSION_TERMINAL_REASON_NONE)
    {
        return;
    }

    if (packet.runtimeSessionToken != 0
        && packet.runtimeSessionToken == ms_lastAppliedTerminalRuntimeSessionToken
        && packet.terminalReasonCode == ms_lastAppliedTerminalReasonCode)
    {
        return;
    }

    ms_lastAppliedTerminalRuntimeSessionToken = packet.runtimeSessionToken;
    ms_lastAppliedTerminalReasonCode = packet.terminalReasonCode;

    if (packet.terminalReasonCode == CPackets::MISSION_TERMINAL_REASON_PASS)
    {
        HandleSubmissionMissionOutcome(true);
    }
    else if (packet.terminalReasonCode == CPackets::MISSION_TERMINAL_REASON_FAIL)
    {
        HandleSubmissionMissionOutcome(false);
    }

    ApplyMissionCleanupFromTerminal();
}

void CMissionSyncState::HandleLocalPlayerRespawned()
{
    EmitParticipantRuntimeEvent(CPackets::MISSION_FLOW_EVENT_RESPAWN_ELIGIBILITY, true);
    EmitParticipantRuntimeEvent(CPackets::MISSION_FLOW_EVENT_PARTICIPANT_RESPAWNED, false);
}

void CMissionSyncState::ProcessSubmissionMissionSync()
{
    ProcessFlowStateDriftCorrection();

    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    CPlayerPed* localPlayer = FindPlayerPed(0);
    if (!localPlayer)
    {
        return;
    }

    uint8_t detectedType = CPackets::SUBMISSION_MISSION_TYPE_MAX;
    if (localPlayer->m_pVehicle)
    {
        detectedType = ResolveSubmissionMissionTypeByVehicleModel(localPlayer->m_pVehicle->m_nModelIndex);
    }

    for (uint8_t mode = 0; mode < CPackets::SUBMISSION_MISSION_TYPE_MAX; ++mode)
    {
        const bool shouldBeActive = mode == detectedType;
        auto it = ms_submissionStates.find(mode);
        if (!shouldBeActive)
        {
            if (it != ms_submissionStates.end() && it->second.active)
            {
                it->second.active = 0;
                it->second.timerMs = 0;
                it->second.checkpointState = {};
                it->second.timerState = {};
                EmitSubmissionMissionDelta(mode, 0, it->second);
            }
            continue;
        }

        SubmissionState& state = ms_submissionStates[mode];
        state.participantCount = CountVehicleParticipants(localPlayer->m_pVehicle);

        if (!state.active)
        {
            state.submissionType = mode;
            state.active = 1;
            state.level = 1;
            state.stage = 1;
            state.progress = 0;
            state.timerMs = 0;
            state.score = 0;
            state.rewardCash = 0;
            state.outcome = 0;
            state.checkpointState.currentIndex = ms_sideContentAttemptState.checkpointIndex;
            state.checkpointState.totalCount = ms_sideContentAttemptState.checkpointCount;
            state.checkpointState.activeFlag = ms_sideContentAttemptState.checkpointCount > 0 ? 1 : 0;
            state.timerState.startTick = GetTickCount();
            state.timerState.remaining = std::max(ms_sideContentAttemptState.timerMs, 0);
            state.timerState.paused = ms_sideContentAttemptState.timerFrozen ? 1 : 0;
            state.timerMs = state.timerState.remaining;
            state.lastBroadcastSecond = -1;
            EmitSubmissionMissionDelta(mode, 0, state);
            continue;
        }

        state.checkpointState.currentIndex = ms_sideContentAttemptState.checkpointIndex;
        state.checkpointState.totalCount = ms_sideContentAttemptState.checkpointCount;
        state.checkpointState.activeFlag = ms_sideContentAttemptState.checkpointCount > 0 ? 1 : 0;
        state.timerState.startTick = GetTickCount();
        state.timerState.remaining = std::max(ms_sideContentAttemptState.timerMs, 0);
        state.timerState.paused = ms_sideContentAttemptState.timerFrozen ? 1 : 0;
        state.timerMs = state.timerState.remaining;

        const int32_t timerSecond = state.timerState.remaining / 1000;
        if (timerSecond != state.lastBroadcastSecond)
        {
            state.lastBroadcastSecond = timerSecond;
            EmitSubmissionMissionDelta(mode, 0, state);
        }
    }
}

void CMissionSyncState::HandleSubmissionMissionSnapshotBegin(const CPackets::SubmissionMissionSnapshotBegin& packet)
{
    ms_submissionSnapshotInProgress = true;
    ms_submissionSnapshotVersion = packet.snapshotVersion;
    ms_submissionStates.clear();
}

void CMissionSyncState::HandleSubmissionMissionSnapshotEntry(const CPackets::SubmissionMissionSnapshotEntry& packet)
{
    SubmissionState state{};
    state.submissionType = packet.submissionType;
    state.active = packet.active;
    state.level = packet.level;
    state.stage = packet.stage;
    state.progress = packet.progress;
    state.timerMs = packet.timerMs;
    state.score = packet.score;
    state.rewardCash = packet.rewardCash;
    state.outcome = packet.outcome;
    state.participantCount = packet.participantCount;
    state.currArea = packet.currArea;
    state.checkpointState = packet.checkpointState;
    state.timerState = packet.timerState;
    state.stateTimestampMs = packet.stateTimestampMs;
    state.stateVersion = packet.stateVersion;
    ms_submissionStates[packet.submissionType] = state;
}

void CMissionSyncState::HandleSubmissionMissionSnapshotEnd(const CPackets::SubmissionMissionSnapshotEnd& packet)
{
    ms_submissionSnapshotVersion = packet.snapshotVersion;
    ms_submissionSnapshotInProgress = false;
}

void CMissionSyncState::HandleSubmissionMissionStateDelta(const CPackets::SubmissionMissionStateDelta& packet)
{
    if (packet.action != 0)
    {
        ms_submissionStates.erase(packet.submissionType);
        return;
    }

    auto it = ms_submissionStates.find(packet.submissionType);
    if (it != ms_submissionStates.end() && packet.stateVersion < it->second.stateVersion)
    {
        return;
    }

    HandleSubmissionMissionSnapshotEntry(packet);
}

void CMissionSyncState::TriggerSubmissionMissionRewardDelta(int rewardDelta)
{
    if (!CLocalPlayer::m_bIsHost || rewardDelta == 0)
    {
        return;
    }

    CPlayerPed* localPlayer = FindPlayerPed(0);
    if (!localPlayer || !localPlayer->m_pVehicle)
    {
        return;
    }

    const uint8_t submissionType = ResolveSubmissionMissionTypeByVehicleModel(localPlayer->m_pVehicle->m_nModelIndex);
    if (submissionType >= CPackets::SUBMISSION_MISSION_TYPE_MAX)
    {
        return;
    }

    SubmissionState& state = ms_submissionStates[submissionType];
    state.submissionType = submissionType;
    state.active = 1;
    state.level = std::max<uint8_t>(state.level, 1);
    state.stage = std::max<uint8_t>(state.stage, 1);
    state.progress = static_cast<uint16_t>(std::min<int>(UINT16_MAX, static_cast<int>(state.progress) + 1));
    state.score = std::max<int32_t>(0, state.score + rewardDelta);
    state.rewardCash += rewardDelta;
    state.outcome = 0;
    state.participantCount = CountVehicleParticipants(localPlayer->m_pVehicle);
    state.checkpointState.currentIndex = ms_sideContentAttemptState.checkpointIndex;
    state.checkpointState.totalCount = ms_sideContentAttemptState.checkpointCount;
    state.checkpointState.activeFlag = ms_sideContentAttemptState.checkpointCount > 0 ? 1 : 0;
    state.timerState.startTick = GetTickCount();
    state.timerState.remaining = std::max(ms_sideContentAttemptState.timerMs, 0);
    state.timerState.paused = ms_sideContentAttemptState.timerFrozen ? 1 : 0;
    state.timerMs = state.timerState.remaining;
    EmitSubmissionMissionDelta(submissionType, 0, state);
}

void CMissionSyncState::HandleSubmissionMissionOutcome(bool passed)
{
    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    for (auto& [submissionType, state] : ms_submissionStates)
    {
        if (!state.active)
        {
            continue;
        }

        state.active = 0;
        state.outcome = passed ? 1 : 2;
        EmitSubmissionMissionDelta(submissionType, 0, state);
        break;
    }
}

bool CMissionSyncState::HandleNetworkSwitchWidescreen(bool enabled)
{
    if (CLocalPlayer::m_bIsHost)
    {
        return true;
    }

    if (!IsMissionContextActive())
    {
        return false;
    }

    if (!ms_bScriptWideScreenForced)
    {
        ms_bUserWideScreenPreference = TheCamera.m_bWideScreenOn;
        ms_bHasUserWideScreenPreference = true;
    }

    if (ms_nLastAppliedWidescreenEventId == ms_nWidescreenEventId
        && ms_bLastAppliedWidescreenValue == enabled)
    {
        return false;
    }

    ms_bScriptWideScreenForced = true;
    ms_bScriptWideScreenValue = enabled;
    ms_nLastAppliedWidescreenEventId = ms_nWidescreenEventId;
    ms_bLastAppliedWidescreenValue = enabled;

    ApplyWideScreen(enabled);
    return false;
}

bool CMissionSyncState::HandleOpCodePreExecute(uint16_t opcode)
{
    if (opcode == OP_SWITCH_WIDESCREEN && !IsMissionContextActive())
    {
        return false;
    }

    for (const auto& entry : kPreExecuteOpCodeHandlers)
    {
        if (entry.opcode == opcode)
        {
            return entry.handler();
        }
    }

    if (!CLocalPlayer::m_bIsHost
        && IsMissionContextActive()
        && IsAuthoritativeObjectiveRenderOpcode(opcode))
    {
        return false;
    }

    if (!CLocalPlayer::m_bIsHost
        && IsMissionContextActive()
        && (opcode == 0x01B4
            || opcode == OP_SET_PLAYER_FIRE_BUTTON
            || opcode == OP_DISPLAY_HUD
            || opcode == OP_SET_CAMERA_CONTROL))
    {
        return false;
    }

    return true;
}

bool CMissionSyncState::HandleEndSceneSkip()
{
    if (ms_pendingConsensusCutsceneSkipToken == 0)
    {
        return false;
    }

    ms_pendingConsensusCutsceneSkipToken = 0;
    ms_bCutsceneActive = false;
    ms_nWidescreenEventId++;
    ResetScriptWideScreenOverride();
    CHud::m_BigMessage[1][0] = 0;
    return true;
}

bool CMissionSyncState::HandleStartCutscene()
{
    if (!CLocalPlayer::m_bIsHost)
    {
        if (ms_pendingReplicatedCutscenePhase != CUTSCENE_PHASE_INTRO)
        {
            return false;
        }
        ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_NONE;
    }

    ms_bCutsceneActive = true;
    ms_nWidescreenEventId++;

    if (CCutsceneMgr::ms_cutsceneLoadStatus == 2)
    {
        return true;
    }

    ms_bLoadingCutscene = true;
    return false;
}

bool CMissionSyncState::HandleClearCutscene()
{
    if (!CLocalPlayer::m_bIsHost)
    {
        if (ms_pendingReplicatedCutscenePhase != CUTSCENE_PHASE_END)
        {
            return false;
        }
        ms_pendingReplicatedCutscenePhase = CUTSCENE_PHASE_NONE;
    }

    if (CLocalPlayer::m_bIsHost)
    {
        EmitMissionFlowCutscenePhase(CUTSCENE_PHASE_END, CCutsceneMgr::ms_cutsceneName, (uint8_t)CGame::currArea, 0);
    }

    ms_bCutsceneActive = false;
    ms_nWidescreenEventId++;
    ResetScriptWideScreenOverride();
    return true;
}

void CMissionSyncState::HandleAuthoritativeCutsceneSkip(uint32_t sessionToken)
{
    if (sessionToken != 0 && sessionToken == ms_lastAuthoritativeCutsceneSkipToken)
    {
        return;
    }

    ms_lastAuthoritativeCutsceneSkipToken = sessionToken;
    ms_pendingConsensusCutsceneSkipToken = sessionToken != 0 ? sessionToken : 1;
    ms_bCutsceneActive = false;
    ms_sideContentAttemptState.cutscenePhase = CUTSCENE_PHASE_SKIP;
    ms_sideContentAttemptState.cutsceneSessionToken = sessionToken;
    EmitMissionFlowCutscenePhase(CUTSCENE_PHASE_SKIP, CCutsceneMgr::ms_cutsceneName, (uint8_t)CGame::currArea, sessionToken);
}
