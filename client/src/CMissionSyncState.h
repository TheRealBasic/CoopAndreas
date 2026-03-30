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
    static void SetMissionAuthorityEpoch(uint32_t missionEpoch);
    static void SetMissionAuthorityMetadata(uint32_t missionEpoch, uint32_t sequenceId);
    static uint32_t GetMissionAuthorityEpoch();
    static uint32_t NextMissionEventSequenceId();
    static bool ShouldAcceptInboundMissionEvent(uint32_t missionEpoch, uint32_t sequenceId);
    static void EmitMissionFlowCutscenePhase(uint8_t phase, const char* cutsceneName, uint8_t currArea, uint32_t cutsceneSessionToken);
    static void EmitMissionFlowText(uint16_t opcode, const HostTextMessage& message);
    static void EmitMissionFlowOpcode(uint16_t opcode, const int* params, uint16_t paramCount, const char* text);
    static void EmitParticipantRuntimeEvent(uint8_t eventType, bool respawnEligible);
    static void HandleMissionFlowSync(const CPackets::MissionFlowSync& packet);
    static void HandleMissionRuntimeSnapshotBegin(const CPackets::MissionRuntimeSnapshotBegin& packet);
    static void HandleMissionRuntimeSnapshotState(const CPackets::MissionRuntimeSnapshotState& packet);
    static void HandleMissionRuntimeSnapshotActor(const CPackets::MissionRuntimeSnapshotActor& packet);
    static void HandleMissionRuntimeSnapshotEnd(const CPackets::MissionRuntimeSnapshotEnd& packet);
    static bool CanAcceptLiveMissionEvents();
    static void HandleAuthoritativeCutsceneSkip(uint32_t sessionToken);
    static bool HandleNetworkSwitchWidescreen(bool enabled);
    static void ProcessSubmissionMissionSync();
    static void HandleSubmissionMissionSnapshotBegin(const CPackets::SubmissionMissionSnapshotBegin& packet);
    static void HandleSubmissionMissionSnapshotEntry(const CPackets::SubmissionMissionSnapshotEntry& packet);
    static void HandleSubmissionMissionSnapshotEnd(const CPackets::SubmissionMissionSnapshotEnd& packet);
    static void HandleSubmissionMissionStateDelta(const CPackets::SubmissionMissionStateDelta& packet);
    static void TriggerSubmissionMissionRewardDelta(int rewardDelta);
    static void HandleSubmissionMissionOutcome(bool passed);
    static void ApplyAdjudicatedTerminalState(const CPackets::MissionFlowSync& packet);
    static void HandleLocalPlayerRespawned();
    static uint16_t GetMissionInstanceId();

    static bool HandleEndSceneSkip();
    static bool HandleStartCutscene();
    static bool HandleClearCutscene();
};
