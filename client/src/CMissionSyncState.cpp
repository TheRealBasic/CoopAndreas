#include "stdafx.h"
#include "CMissionSyncState.h"

#include "CCutsceneMgr.h"
#include "CNetwork.h"

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

    bool ms_bScriptWideScreenForced = false;
    bool ms_bScriptWideScreenValue = false;
    bool ms_bUserWideScreenPreference = false;
    bool ms_bHasUserWideScreenPreference = false;

    uint32_t ms_nWidescreenEventId = 1;
    uint32_t ms_nLastAppliedWidescreenEventId = 0;
    bool ms_bLastAppliedWidescreenValue = false;
    uint32_t ms_nMissionFlowSequence = 0;
    uint32_t ms_submissionStateVersion = 0;
    uint32_t ms_submissionSnapshotVersion = 0;
    bool ms_submissionSnapshotInProgress = false;

    struct SubmissionState
    {
        uint8_t submissionType = 0;
        uint8_t active = 0;
        uint8_t stage = 0;
        uint16_t progress = 0;
        int32_t timerMs = 0;
        int32_t rewardCash = 0;
        uint8_t currArea = 0;
        uint64_t stateTimestampMs = 0;
        uint32_t stateVersion = 0;
    };

    std::unordered_map<uint8_t, SubmissionState> ms_submissionStates{};

    constexpr uint16_t OP_END_SCENE_SKIP = 0x0701;
    constexpr uint16_t OP_START_CUTSCENE = 0x02E7;
    constexpr uint16_t OP_CLEAR_CUTSCENE = 0x02EA;
    constexpr uint16_t OP_SWITCH_WIDESCREEN = 0x02A3;

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

    uint8_t ResolveSubmissionMissionTypeByVehicleModel(int modelId)
    {
        switch (modelId)
        {
        case MODEL_TAXI:
        case MODEL_CABBIE:
            return CPackets::SUBMISSION_MISSION_TAXI;
        case MODEL_FIRETRUK:
            return CPackets::SUBMISSION_MISSION_FIREFIGHTER;
        case MODEL_AMBULAN:
            return CPackets::SUBMISSION_MISSION_PARAMEDIC;
        case MODEL_BROADWAY:
            return CPackets::SUBMISSION_MISSION_PIMP;
        case MODEL_FREIGHT:
        case MODEL_STREAK:
            return CPackets::SUBMISSION_MISSION_FREIGHT_TRAIN;
        case MODEL_COPCARLA:
        case MODEL_COPCARRU:
        case MODEL_COPCARSF:
        case MODEL_COPBIKE:
        case MODEL_FBIRANCH:
        case MODEL_ENFORCER:
        case MODEL_RHINO:
            return CPackets::SUBMISSION_MISSION_VIGILANTE;
        default:
            return CPackets::SUBMISSION_MISSION_TYPE_MAX;
        }
    }

    void EmitSubmissionMissionDelta(uint8_t submissionType, uint8_t action, uint8_t active, uint8_t stage, uint16_t progress, int32_t timerMs, int32_t rewardCash)
    {
        CPackets::SubmissionMissionStateDelta packet{};
        packet.submissionType = submissionType;
        packet.action = action;
        packet.active = active;
        packet.stage = stage;
        packet.progress = progress;
        packet.timerMs = timerMs;
        packet.rewardCash = rewardCash;
        packet.currArea = static_cast<uint8_t>(CGame::currArea);
        packet.stateTimestampMs = GetTickCount64();
        packet.stateVersion = ++ms_submissionStateVersion;

        CNetwork::SendPacket(CPacketsID::SUBMISSION_MISSION_STATE_DELTA, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }
}

void CMissionSyncState::Init()
{
    ms_bLoadingCutscene = true;
    memset(ms_abLoadingMissionAudio, 0, sizeof(ms_abLoadingMissionAudio));
    ms_bProcessingTaskSequence = false;
    ms_bMissionActive = false;
    ms_bCutsceneActive = false;

    ms_bScriptWideScreenForced = false;
    ms_bScriptWideScreenValue = false;
    ms_bUserWideScreenPreference = false;
    ms_bHasUserWideScreenPreference = false;

    ms_nWidescreenEventId = 1;
    ms_nLastAppliedWidescreenEventId = 0;
    ms_bLastAppliedWidescreenValue = false;
    ms_nMissionFlowSequence = 0;
    ms_submissionStateVersion = 0;
    ms_submissionSnapshotVersion = 0;
    ms_submissionSnapshotInProgress = false;
    ms_submissionStates.clear();
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

    if (!onMission)
    {
        ResetScriptWideScreenOverride();
    }
}

void CMissionSyncState::EmitMissionFlowCutsceneTrigger(const char* cutsceneName, uint8_t currArea)
{
    if (!CLocalPlayer::m_bIsHost)
    {
        return;
    }

    CPackets::MissionFlowSync packet{};
    packet.eventType = CPackets::MISSION_FLOW_EVENT_CUTSCENE_TRIGGER;
    packet.currArea = currArea;
    packet.onMission = (CTheScripts::OnAMissionFlag && CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag]) ? 1 : 0;
    packet.sequence = ++ms_nMissionFlowSequence;
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
        packet.eventType = CPackets::MISSION_FLOW_EVENT_PASS;
    }
    else if (opcode == 0x045C)
    {
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
    strncpy_s(packet.gxt, message.gxt ? message.gxt : "", sizeof(packet.gxt));
    CNetwork::SendPacket(CPacketsID::MISSION_FLOW_SYNC, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
}

void CMissionSyncState::HandleMissionFlowSync(const CPackets::MissionFlowSync& packet)
{
    if (CLocalPlayer::m_bIsHost)
    {
        return;
    }

    HandleMissionFlagSync(packet.onMission != 0);

    if (!packet.replay)
    {
        return;
    }

    if (packet.eventType == CPackets::MISSION_FLOW_EVENT_OBJECTIVE
        || packet.eventType == CPackets::MISSION_FLOW_EVENT_FAIL
        || packet.eventType == CPackets::MISSION_FLOW_EVENT_PASS)
    {
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
                EmitSubmissionMissionDelta(mode, 0, 0, it->second.stage, it->second.progress, 0, it->second.rewardCash);
            }
            continue;
        }

        SubmissionState& state = ms_submissionStates[mode];
        if (!state.active)
        {
            state.submissionType = mode;
            state.active = 1;
            state.stage = 1;
            state.progress = 0;
            state.timerMs = 0;
            state.rewardCash = 0;
            EmitSubmissionMissionDelta(mode, 0, state.active, state.stage, state.progress, state.timerMs, state.rewardCash);
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
    state.stage = packet.stage;
    state.progress = packet.progress;
    state.timerMs = packet.timerMs;
    state.rewardCash = packet.rewardCash;
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
    state.stage = std::max<uint8_t>(state.stage, 1);
    state.progress = static_cast<uint16_t>(std::min<int>(UINT16_MAX, static_cast<int>(state.progress) + 1));
    state.rewardCash += rewardDelta;
    state.timerMs = std::max<int32_t>(0, state.timerMs);
    EmitSubmissionMissionDelta(submissionType, 0, state.active, state.stage, state.progress, state.timerMs, state.rewardCash);
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

    return true;
}

bool CMissionSyncState::HandleEndSceneSkip()
{
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
    ms_bCutsceneActive = false;
    ms_nWidescreenEventId++;
    ResetScriptWideScreenOverride();
    return true;
}
