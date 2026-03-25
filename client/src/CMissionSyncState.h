#pragma once

class CMissionSyncState
{
public:
    static void Init();

    static bool IsProcessingTaskSequence();
    static void SetProcessingTaskSequence(bool processing);

    static void MarkMissionAudioLoading(uint8_t slotId);
    static void ProcessMissionAudioLoading();

    static bool HandleOpCodePreExecute(uint16_t opcode);
    static void ProcessDeferredCutsceneStart();

    static bool HandleEndSceneSkip();
    static bool HandleStartCutscene();
};
