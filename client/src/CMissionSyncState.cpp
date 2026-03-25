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

    constexpr uint16_t OP_END_SCENE_SKIP = 0x0701;
    constexpr uint16_t OP_START_CUTSCENE = 0x02E7;

    constexpr OpCodeHandlerEntry kPreExecuteOpCodeHandlers[] = {
        { OP_END_SCENE_SKIP, &CMissionSyncState::HandleEndSceneSkip },
        { OP_START_CUTSCENE, &CMissionSyncState::HandleStartCutscene }
    };
}

void CMissionSyncState::Init()
{
    ms_bLoadingCutscene = true;
    memset(ms_abLoadingMissionAudio, 0, sizeof(ms_abLoadingMissionAudio));
    ms_bProcessingTaskSequence = false;
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

bool CMissionSyncState::HandleOpCodePreExecute(uint16_t opcode)
{
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
    CHud::m_BigMessage[1][0] = 0;
    return true;
}

bool CMissionSyncState::HandleStartCutscene()
{
    if (CCutsceneMgr::ms_cutsceneLoadStatus == 2)
    {
        return true;
    }

    ms_bLoadingCutscene = true;
    return false;
}
