#pragma once
#include "CPackets.h"

class CMissionSyncState
{
public:
    struct HostTextMessage
    {
        uint8_t messageType;
        uint32_t time;
        uint8_t flag;
        const char* gxt;
    };

    static void Init();

    static bool IsProcessingTaskSequence();
    static void SetProcessingTaskSequence(bool processing);

    static void MarkMissionAudioLoading(uint8_t slotId);
    static void ProcessMissionAudioLoading();

    static bool HandleOpCodePreExecute(uint16_t opcode);
    static void ProcessDeferredCutsceneStart();
    static void ProcessWidescreenPolicy();

    static void HandleMissionFlagSync(bool onMission);
    static void EmitMissionFlowCutsceneTrigger(const char* cutsceneName, uint8_t currArea);
    static void EmitMissionFlowText(uint16_t opcode, const HostTextMessage& message);
    static void HandleMissionFlowSync(const CPackets::MissionFlowSync& packet);
    static bool HandleNetworkSwitchWidescreen(bool enabled);
    static void ProcessSubmissionMissionSync();
    static void HandleSubmissionMissionSnapshotBegin(const CPackets::SubmissionMissionSnapshotBegin& packet);
    static void HandleSubmissionMissionSnapshotEntry(const CPackets::SubmissionMissionSnapshotEntry& packet);
    static void HandleSubmissionMissionSnapshotEnd(const CPackets::SubmissionMissionSnapshotEnd& packet);
    static void HandleSubmissionMissionStateDelta(const CPackets::SubmissionMissionStateDelta& packet);
    static void TriggerSubmissionMissionRewardDelta(int rewardDelta);

    static bool HandleEndSceneSkip();
    static bool HandleStartCutscene();
    static bool HandleClearCutscene();
};
