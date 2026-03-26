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
    static void ProcessWidescreenPolicy();

    static void HandleMissionFlagSync(bool onMission);
    static bool HandleNetworkSwitchWidescreen(bool enabled);

    static bool HandleEndSceneSkip();
    static bool HandleStartCutscene();
    static bool HandleClearCutscene();
};
