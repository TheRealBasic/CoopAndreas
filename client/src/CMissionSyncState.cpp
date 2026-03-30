#include "stdafx.h"
#include "CMissionSyncState.h"

#include "CCutsceneMgr.h"
#include "CNetwork.h"
#include "ObjectiveSyncState.h"

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

    bool ms_bScriptWideScreenForced = false;
    bool ms_bScriptWideScreenValue = false;
    bool ms_bUserWideScreenPreference = false;
    bool ms_bHasUserWideScreenPreference = false;

    uint32_t ms_nWidescreenEventId = 1;
    uint32_t ms_nLastAppliedWidescreenEventId = 0;
    bool ms_bLastAppliedWidescreenValue = false;
    uint32_t ms_nMissionFlowSequence = 0;
    uint32_t ms_nLastReceivedMissionFlowSequence = 0;
    uint32_t ms_lastAppliedTerminalRuntimeSessionToken = 0;
    uint8_t ms_lastAppliedTerminalReasonCode = CPackets::MISSION_TERMINAL_REASON_NONE;
    uint16_t ms_runtimeMissionInstanceId = 0;
    uint32_t ms_submissionStateVersion = 0;
    uint32_t ms_submissionSnapshotVersion = 0;
    bool ms_submissionSnapshotInProgress = false;
    // Shared side-content replication state (schools/races/courier/etc).
    //
    // Payload contract:
    // - missionId: start/launch identity (Mission.LoadAndLaunchInternal, race starts that launch mission scripts).
    // - objective: current objective/help text key.
    // - checkpointIndex: monotonically increasing progression index (checkpoint create/update + scripted objective bumps).
    // - timerVisible/timerFrozen/timerMs: HUD timer parity for timer-driven side content.
    // - passFailPending: deterministic terminal outcome latch (1 pass / 2 fail) to avoid duplicate outcomes.
    // - playerControlState: replicated player-control gate for scripted start/end transitions.
    //
    // Server stores and replays the latest MissionFlowSync payload so reconnecting and late-join peers
    // immediately hydrate this state without waiting for another opcode edge.
    using SideContentAttemptState = ObjectiveSync::State;
    SideContentAttemptState ms_sideContentAttemptState{};

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
        uint64_t stateTimestampMs = 0;
        uint32_t stateVersion = 0;
        uint64_t startedAtMs = 0;
        int32_t lastBroadcastSecond = -1;
    };

    std::unordered_map<uint8_t, SubmissionState> ms_submissionStates{};

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
        const bool firingLocked = state.firingLocked != 0;
        const bool hudHidden = state.hudHidden != 0;

        pad->bDisablePlayerEnterCar = movementLocked ? 1 : 0;
        pad->bDisablePlayerDuck = movementLocked ? 1 : 0;
        pad->bDisablePlayerCycleWeapon = movementLocked ? 1 : 0;
        pad->bDisablePlayerJump = movementLocked ? 1 : 0;
        pad->bDisablePlayerFireWeapon = firingLocked ? 1 : 0;
        pad->bDisablePlayerFireWeaponWithL1 = firingLocked ? 1 : 0;
        pad->bDisablePlayerDisplayVitalStats = hudHidden ? 1 : 0;
    }

    void ApplyMissionCleanupFromTerminal()
    {
        CNetworkCheckpoint::Remove();
        CNetworkEntityBlip::ClearEntityBlips();

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
}

void CMissionSyncState::Init()
{
    ms_bLoadingCutscene = true;
    memset(ms_abLoadingMissionAudio, 0, sizeof(ms_abLoadingMissionAudio));
    ms_bProcessingTaskSequence = false;
    ms_bMissionActive = false;
    ms_bCutsceneActive = false;
    ms_lastAuthoritativeCutsceneSkipToken = 0;

    ms_bScriptWideScreenForced = false;
    ms_bScriptWideScreenValue = false;
    ms_bUserWideScreenPreference = false;
    ms_bHasUserWideScreenPreference = false;

    ms_nWidescreenEventId = 1;
    ms_nLastAppliedWidescreenEventId = 0;
    ms_bLastAppliedWidescreenValue = false;
    ms_nMissionFlowSequence = 0;
    ms_nLastReceivedMissionFlowSequence = 0;
    ms_lastAppliedTerminalRuntimeSessionToken = 0;
    ms_lastAppliedTerminalReasonCode = CPackets::MISSION_TERMINAL_REASON_NONE;
    ms_runtimeMissionInstanceId = 0;
    ms_submissionStateVersion = 0;
    ms_submissionSnapshotVersion = 0;
    ms_submissionSnapshotInProgress = false;
    ms_submissionStates.clear();
    ms_sideContentAttemptState = {};
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
    packet.sequence = ++ms_nMissionFlowSequence;
    packet.missionId = ms_sideContentAttemptState.missionId;
    packet.timerMs = ms_sideContentAttemptState.timerMs;
    packet.objectivePhaseIndex = ms_sideContentAttemptState.objectivePhaseIndex;
    packet.checkpointIndex = ms_sideContentAttemptState.checkpointIndex;
    packet.checkpointCount = ms_sideContentAttemptState.checkpointCount;
    packet.timerVisible = ms_sideContentAttemptState.timerVisible;
    packet.timerFrozen = ms_sideContentAttemptState.timerFrozen;
    packet.timerDirection = ms_sideContentAttemptState.timerDirection;
    packet.passFailPending = ms_sideContentAttemptState.passFailPending;
    packet.playerControlState = ms_sideContentAttemptState.playerControlState;
    packet.movementLocked = ms_sideContentAttemptState.movementLocked;
    packet.firingLocked = ms_sideContentAttemptState.firingLocked;
    packet.cameraLocked = ms_sideContentAttemptState.cameraLocked;
    packet.hudHidden = ms_sideContentAttemptState.hudHidden;
    packet.cutscenePhase = phase;
    packet.cutsceneSessionToken = cutsceneSessionToken;
    strncpy_s(packet.objective, ms_sideContentAttemptState.objective, sizeof(packet.objective));
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
    packet.sequence = ++ms_nMissionFlowSequence;
    packet.sourceOpcode = opcode;
    packet.missionId = ms_sideContentAttemptState.missionId;
    packet.timerMs = ms_sideContentAttemptState.timerMs;
    packet.objectivePhaseIndex = ms_sideContentAttemptState.objectivePhaseIndex;
    packet.checkpointIndex = ms_sideContentAttemptState.checkpointIndex;
    packet.checkpointCount = ms_sideContentAttemptState.checkpointCount;
    packet.timerVisible = ms_sideContentAttemptState.timerVisible;
    packet.timerFrozen = ms_sideContentAttemptState.timerFrozen;
    packet.timerDirection = ms_sideContentAttemptState.timerDirection;
    packet.passFailPending = ms_sideContentAttemptState.passFailPending;
    packet.playerControlState = ms_sideContentAttemptState.playerControlState;
    packet.movementLocked = ms_sideContentAttemptState.movementLocked;
    packet.firingLocked = ms_sideContentAttemptState.firingLocked;
    packet.cameraLocked = ms_sideContentAttemptState.cameraLocked;
    packet.hudHidden = ms_sideContentAttemptState.hudHidden;
    packet.cutscenePhase = ms_sideContentAttemptState.cutscenePhase;
    packet.cutsceneSessionToken = ms_sideContentAttemptState.cutsceneSessionToken;
    strncpy_s(packet.objective, ms_sideContentAttemptState.objective, sizeof(packet.objective));
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
    packet.sequence = ++ms_nMissionFlowSequence;
    packet.sourceOpcode = opcode;
    packet.missionId = ms_sideContentAttemptState.missionId;
    packet.timerMs = ms_sideContentAttemptState.timerMs;
    packet.objectivePhaseIndex = ms_sideContentAttemptState.objectivePhaseIndex;
    packet.checkpointIndex = ms_sideContentAttemptState.checkpointIndex;
    packet.checkpointCount = ms_sideContentAttemptState.checkpointCount;
    packet.timerVisible = ms_sideContentAttemptState.timerVisible;
    packet.timerFrozen = ms_sideContentAttemptState.timerFrozen;
    packet.timerDirection = ms_sideContentAttemptState.timerDirection;
    packet.passFailPending = ms_sideContentAttemptState.passFailPending;
    packet.playerControlState = ms_sideContentAttemptState.playerControlState;
    packet.movementLocked = ms_sideContentAttemptState.movementLocked;
    packet.firingLocked = ms_sideContentAttemptState.firingLocked;
    packet.cameraLocked = ms_sideContentAttemptState.cameraLocked;
    packet.hudHidden = ms_sideContentAttemptState.hudHidden;
    packet.cutscenePhase = ms_sideContentAttemptState.cutscenePhase;
    packet.cutsceneSessionToken = ms_sideContentAttemptState.cutsceneSessionToken;
    strncpy_s(packet.objective, ms_sideContentAttemptState.objective, sizeof(packet.objective));
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::HandleMissionFlowSync(const CPackets::MissionFlowSync& packet)
{
    if (CLocalPlayer::m_bIsHost)
    {
        return;
    }

    if (packet.sequence != 0 && packet.sequence <= ms_nLastReceivedMissionFlowSequence)
    {
        return;
    }

    ms_nLastReceivedMissionFlowSequence = packet.sequence;
    HandleMissionFlagSync(packet.onMission != 0);

    const bool newAttempt = packet.sourceOpcode == 0x0417
        || (packet.missionId != 0 && packet.missionId != ms_sideContentAttemptState.missionId);
    if (newAttempt)
    {
        ms_sideContentAttemptState = {};
    }

    ms_sideContentAttemptState.missionId = packet.missionId;
    ms_sideContentAttemptState.timerMs = std::max(packet.timerMs, 0);
    ms_sideContentAttemptState.objectivePhaseIndex = packet.objectivePhaseIndex;
    ms_sideContentAttemptState.checkpointIndex = packet.checkpointIndex;
    ms_sideContentAttemptState.checkpointCount = packet.checkpointCount;
    ms_sideContentAttemptState.timerVisible = packet.timerVisible;
    ms_sideContentAttemptState.timerFrozen = packet.timerFrozen;
    ms_sideContentAttemptState.timerDirection = packet.timerDirection;
    ms_sideContentAttemptState.playerControlState = packet.playerControlState;
    ms_sideContentAttemptState.movementLocked = packet.movementLocked;
    ms_sideContentAttemptState.firingLocked = packet.firingLocked;
    ms_sideContentAttemptState.cameraLocked = packet.cameraLocked;
    ms_sideContentAttemptState.hudHidden = packet.hudHidden;
    ms_sideContentAttemptState.cutscenePhase = packet.cutscenePhase;
    ms_sideContentAttemptState.cutsceneSessionToken = packet.cutsceneSessionToken;
    if (packet.objective[0])
    {
        strncpy_s(ms_sideContentAttemptState.objective, packet.objective, sizeof(ms_sideContentAttemptState.objective));
    }
    else
    {
        memset(ms_sideContentAttemptState.objective, 0, sizeof(ms_sideContentAttemptState.objective));
    }

    if (ms_sideContentAttemptState.passFailPending == 0)
    {
        ms_sideContentAttemptState.passFailPending = packet.passFailPending;
    }

    ApplyAdjudicatedTerminalState(packet);

    if (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_INTRO)
    {
        ms_bCutsceneActive = true;
    }
    else if (packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_SKIP || packet.eventType == CPackets::MISSION_FLOW_EVENT_CUTSCENE_END)
    {
        ms_bCutsceneActive = false;
        ResetScriptWideScreenOverride();
    }

    ApplyReplicatedControlLocks(ms_sideContentAttemptState);

    if (packet.eventType == CPackets::MISSION_FLOW_EVENT_OBJECTIVE
        || packet.eventType == CPackets::MISSION_FLOW_EVENT_FAIL
        || packet.eventType == CPackets::MISSION_FLOW_EVENT_PASS)
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

void CMissionSyncState::ProcessSubmissionMissionSync()
{
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
            state.startedAtMs = GetTickCount64();
            state.lastBroadcastSecond = -1;
            EmitSubmissionMissionDelta(mode, 0, state);
            continue;
        }

        if (state.startedAtMs == 0)
        {
            state.startedAtMs = GetTickCount64();
        }

        const int32_t elapsedMs = static_cast<int32_t>(GetTickCount64() - state.startedAtMs);
        const int32_t elapsedSeconds = elapsedMs / 1000;
        state.timerMs = std::max<int32_t>(0, elapsedMs);
        if (elapsedSeconds != state.lastBroadcastSecond)
        {
            state.lastBroadcastSecond = elapsedSeconds;
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
    state.timerMs = std::max<int32_t>(0, state.timerMs);
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
    if (CLocalPlayer::m_bIsHost)
    {
        EmitMissionFlowCutscenePhase(CUTSCENE_PHASE_SKIP, CCutsceneMgr::ms_cutsceneName, (uint8_t)CGame::currArea, 0);
    }

    ms_bCutsceneActive = false;
    ms_nWidescreenEventId++;
    ResetScriptWideScreenOverride();
    CHud::m_BigMessage[1][0] = 0;
    return true;
}

bool CMissionSyncState::HandleStartCutscene()
{
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
    ms_bCutsceneActive = false;
    ms_sideContentAttemptState.cutscenePhase = CUTSCENE_PHASE_SKIP;
    ms_sideContentAttemptState.cutsceneSessionToken = sessionToken;
    EmitMissionFlowCutscenePhase(CUTSCENE_PHASE_SKIP, CCutsceneMgr::ms_cutsceneName, (uint8_t)CGame::currArea, sessionToken);
}
