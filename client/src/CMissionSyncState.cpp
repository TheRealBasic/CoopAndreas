#include "stdafx.h"
#include "CMissionSyncState.h"

#include "CCutsceneMgr.h"

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
