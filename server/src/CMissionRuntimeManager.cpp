#include "CMissionRuntimeManager.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "CNetwork.h"
#include "ObjectiveSyncState.h"
#include "CPlayer.h"

namespace
{
    using RuntimeState = CMissionRuntimeManager::RuntimeState;
    constexpr uint8_t MISSION_FLOW_EVENT_PARTICIPANT_DEATH = 8;
    constexpr uint8_t MISSION_FLOW_EVENT_PARTICIPANT_INCAPACITATED = 9;
    constexpr uint8_t MISSION_FLOW_EVENT_STEALTH_VISION = 13;
    constexpr uint8_t MISSION_FLOW_EVENT_STEALTH_HEARING = 14;
    constexpr uint8_t MISSION_FLOW_EVENT_STEALTH_ALARM = 15;

    RuntimeState g_runtimeState = RuntimeState::Idle;
    CMissionRuntimeManager::MissionFlowPayload g_lastFlow{};
    CMissionRuntimeManager::CheckpointPayload g_lastCheckpoint{};
    bool g_hasFlow = false;
    bool g_hasCheckpoint = false;
    bool g_isOnMission = false;
    bool g_terminalEventSeen = false;
    bool g_terminalOutcomeCommitted = false;
    uint8_t g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
    uint8_t g_terminalSourceEventType = 0;
    uint16_t g_terminalSourceOpcode = 0;
    uint32_t g_terminalSourceSequence = 0;
    uint32_t g_lastSequence = 0;
    uint16_t g_objectiveVersion = 0;
    uint16_t g_checkpointVersion = 0;
    uint32_t g_runtimeSessionToken = 0;
    uint32_t g_missionEpoch = 1;
    uint32_t g_snapshotVersion = 0;
    int g_missionAuthorityPlayerId = -1;
    ObjectiveSync::State g_objectiveState{};
    std::vector<CMissionRuntimePackets::MissionRuntimeSnapshotActor> g_snapshotActors{};
    struct RegistryActorKey
    {
        uint32_t missionEpoch = 0;
        uint32_t scriptLocalIdentifier = 0;
        uint16_t opcode = 0;
        uint8_t slot = 0;
        uint8_t entityType = 0;

        bool operator==(const RegistryActorKey& rhs) const
        {
            return missionEpoch == rhs.missionEpoch
                && scriptLocalIdentifier == rhs.scriptLocalIdentifier
                && opcode == rhs.opcode
                && slot == rhs.slot
                && entityType == rhs.entityType;
        }
    };

    struct RegistryActorKeyHash
    {
        size_t operator()(const RegistryActorKey& key) const
        {
            return std::hash<uint64_t>{}(
                (uint64_t)key.missionEpoch << 32
                | (uint64_t)key.scriptLocalIdentifier
                | (uint64_t)key.opcode);
        }
    };

    std::unordered_map<RegistryActorKey, CMissionRuntimePackets::MissionRuntimeSnapshotActor, RegistryActorKeyHash> g_registryActors{};
    struct MissionRuntimePolicy
    {
        uint8_t respawnEligible = 0;
        uint8_t missionFailThreshold = 1;
        uint8_t incapacitationFailThreshold = UINT8_MAX;
    };

    const std::unordered_map<uint16_t, MissionRuntimePolicy> kMissionPolicyOverrides = {
        // Mission-specific policy overrides can be added here.
    };
    
    struct MigrationSnapshot
    {
        RuntimeState runtimeState = RuntimeState::Idle;
        CMissionRuntimeManager::MissionFlowPayload lastFlow{};
        CMissionRuntimeManager::CheckpointPayload lastCheckpoint{};
        bool hasFlow = false;
        bool hasCheckpoint = false;
        bool isOnMission = false;
        bool terminalEventSeen = false;
        bool terminalOutcomeCommitted = false;
        uint8_t terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
        uint8_t terminalSourceEventType = 0;
        uint16_t terminalSourceOpcode = 0;
        uint32_t terminalSourceSequence = 0;
        uint16_t objectiveVersion = 0;
        uint16_t checkpointVersion = 0;
        uint32_t runtimeSessionToken = 0;
        uint32_t lastSequence = 0;
        ObjectiveSync::State objectiveState{};
    };

    bool IsTerminal(RuntimeState state)
    {
        return state == RuntimeState::Teardown;
    }

    MissionRuntimePolicy ResolveMissionRuntimePolicy(uint16_t missionId)
    {
        auto overrideIt = kMissionPolicyOverrides.find(missionId);
        if (overrideIt != kMissionPolicyOverrides.end())
        {
            return overrideIt->second;
        }
        return MissionRuntimePolicy{};
    }

    bool HasCommittedTerminalOutcome()
    {
        return g_terminalOutcomeCommitted
            && (g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS
                || g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL);
    }

    bool ApplyStealthTransitionAuthoritative(
        uint8_t nextState,
        uint8_t sourceMask,
        uint8_t* stateOut = nullptr,
        uint8_t* sourceMaskOut = nullptr,
        uint16_t* sequenceOut = nullptr,
        uint8_t* modifierFlagsOut = nullptr)
    {
        ObjectiveSync::State candidate = g_objectiveState;
        if (!ObjectiveSync::ApplyStealthStateTransition(candidate, nextState, sourceMask))
        {
            if (stateOut) *stateOut = g_objectiveState.stealthState;
            if (sourceMaskOut) *sourceMaskOut = g_objectiveState.detectionSourceMask;
            if (sequenceOut) *sequenceOut = g_objectiveState.stealthStateSequence;
            if (modifierFlagsOut) *modifierFlagsOut = g_objectiveState.objectiveModifierFlags;
            return false;
        }

        g_objectiveState.stealthState = candidate.stealthState;
        g_objectiveState.detectionSourceMask = candidate.detectionSourceMask;
        g_objectiveState.stealthStateSequence = candidate.stealthStateSequence;
        g_objectiveState.objectiveModifierFlags = candidate.objectiveModifierFlags;

        if (stateOut) *stateOut = g_objectiveState.stealthState;
        if (sourceMaskOut) *sourceMaskOut = g_objectiveState.detectionSourceMask;
        if (sequenceOut) *sequenceOut = g_objectiveState.stealthStateSequence;
        if (modifierFlagsOut) *modifierFlagsOut = g_objectiveState.objectiveModifierFlags;
        return true;
    }

    void ApplyStealthFromEvent(const CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        if (packet.passFailPending == 2 || packet.eventType == 3)
        {
            ApplyStealthTransitionAuthoritative(ObjectiveSync::State::STEALTH_STATE_FAILED, ObjectiveSync::State::DETECTION_SOURCE_NONE);
            return;
        }

        if (packet.eventType == MISSION_FLOW_EVENT_STEALTH_VISION || packet.sourceOpcode == 0x0753)
        {
            const uint8_t nextStealthState =
                packet.stealthState >= ObjectiveSync::State::STEALTH_STATE_ALERTED
                ? ObjectiveSync::State::STEALTH_STATE_ALERTED
                : ObjectiveSync::State::STEALTH_STATE_SUSPICIOUS;
            ApplyStealthTransitionAuthoritative(nextStealthState, ObjectiveSync::State::DETECTION_SOURCE_NPC_VISION);
            return;
        }

        if (packet.eventType == MISSION_FLOW_EVENT_STEALTH_HEARING || packet.sourceOpcode == 0x0610)
        {
            const uint8_t nextStealthState =
                packet.stealthState >= ObjectiveSync::State::STEALTH_STATE_ALERTED
                ? ObjectiveSync::State::STEALTH_STATE_ALERTED
                : ObjectiveSync::State::STEALTH_STATE_SUSPICIOUS;
            ApplyStealthTransitionAuthoritative(nextStealthState, ObjectiveSync::State::DETECTION_SOURCE_NPC_HEARING);
            return;
        }

        if (packet.eventType == MISSION_FLOW_EVENT_STEALTH_ALARM || packet.sourceOpcode == 0x0147)
        {
            ApplyStealthTransitionAuthoritative(ObjectiveSync::State::STEALTH_STATE_ALERTED, ObjectiveSync::State::DETECTION_SOURCE_SCRIPTED_ALARM);
            return;
        }

        // Keep authoritative side synchronized if no transition source fired.
        g_objectiveState.stealthState = packet.stealthState;
        g_objectiveState.detectionSourceMask = packet.detectionSourceMask;
        g_objectiveState.stealthStateSequence = packet.stealthStateSequence;
        g_objectiveState.objectiveModifierFlags = packet.objectiveModifierFlags;
    }

    bool Transition(RuntimeState next)
    {
        if (g_runtimeState == next)
        {
            return false;
        }

        switch (next)
        {
        case RuntimeState::Idle:
            g_runtimeState = next;
            return true;
        case RuntimeState::Starting:
            if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Teardown)
            {
                g_runtimeState = next;
                return true;
            }
            return false;
        case RuntimeState::Active:
            if (g_runtimeState == RuntimeState::Starting || g_runtimeState == RuntimeState::Active)
            {
                g_runtimeState = next;
                return true;
            }
            return false;
        case RuntimeState::PassPending:
        case RuntimeState::FailPending:
            if (IsTerminal(g_runtimeState) || g_terminalEventSeen)
            {
                return false;
            }
            if (g_runtimeState == RuntimeState::Active || g_runtimeState == RuntimeState::Starting)
            {
                g_runtimeState = next;
                g_terminalEventSeen = true;
                return true;
            }
            return false;
        case RuntimeState::Teardown:
            if (IsTerminal(g_runtimeState))
            {
                return false;
            }
            if (g_runtimeState == RuntimeState::PassPending || g_runtimeState == RuntimeState::FailPending || g_runtimeState == RuntimeState::Active || g_runtimeState == RuntimeState::Starting)
            {
                g_runtimeState = next;
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    void ApplyMissionLaunch()
    {
        if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Teardown)
        {
            Transition(RuntimeState::Starting);
            Transition(RuntimeState::Active);
            g_terminalEventSeen = false;
            g_terminalOutcomeCommitted = false;
            g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
            g_terminalSourceEventType = 0;
            g_terminalSourceOpcode = 0;
            g_terminalSourceSequence = 0;
            g_objectiveState = {};
            const MissionRuntimePolicy policy = ResolveMissionRuntimePolicy(0);
            g_objectiveState.respawnEligible = policy.respawnEligible;
            g_objectiveState.missionFailThreshold = policy.missionFailThreshold;
            g_objectiveState.incapacitationFailThreshold = policy.incapacitationFailThreshold;
        }
    }

    void ApplyObjectiveUpdate(const CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        if (!g_hasFlow
            || std::memcmp(g_lastFlow.objective, packet.objective, sizeof(packet.objective)) != 0
            || g_lastFlow.objectivePhaseIndex != packet.objectivePhaseIndex
            || g_lastFlow.objectiveTextToken != packet.objectiveTextToken
            || g_lastFlow.objectiveTextSemantics != packet.objectiveTextSemantics)
        {
            ++g_objectiveVersion;
        }
        if (g_runtimeState == RuntimeState::Starting)
        {
            Transition(RuntimeState::Active);
        }
    }

    void ApplyCheckpointProgression(const CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        if (packet.checkpointIndex > g_lastFlow.checkpointIndex
            || packet.checkpointCount != g_lastFlow.checkpointCount)
        {
            ++g_checkpointVersion;
        }
    }

    bool CommitTerminalOutcome(CMissionRuntimeManager::MissionFlowPayload& packet, uint8_t terminalReasonCode)
    {
        const auto terminalPriority = [](uint8_t reason) -> uint8_t
        {
            switch (reason)
            {
            case CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL: return 3;
            case CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS: return 2;
            case CMissionRuntimePackets::MISSION_TERMINAL_REASON_ON_MISSION_CLEARED: return 1;
            default: return 0;
            }
        };

        if (HasCommittedTerminalOutcome())
        {
            const uint8_t currentPriority = terminalPriority(g_terminalReasonCode);
            const uint8_t incomingPriority = terminalPriority(terminalReasonCode);
            const bool shouldOverride = incomingPriority > currentPriority
                || (incomingPriority == currentPriority && packet.sequence >= g_terminalSourceSequence);
            if (!shouldOverride)
            {
                return false;
            }
        }

        g_terminalEventSeen = true;
        g_terminalOutcomeCommitted = true;
        g_terminalReasonCode = terminalReasonCode;
        g_terminalSourceEventType = packet.eventType;
        g_terminalSourceOpcode = packet.sourceOpcode;
        g_terminalSourceSequence = packet.sequence;
        g_runtimeState = RuntimeState::Teardown;
        g_isOnMission = false;
        g_hasCheckpoint = false;
        g_objectiveState.playerControlState = 1;
        g_objectiveState.movementLocked = 0;
        g_objectiveState.aimingLocked = 0;
        g_objectiveState.firingLocked = 0;
        g_objectiveState.cameraLocked = 0;
        g_objectiveState.hudHidden = 0;

        packet.onMission = 0;
        packet.passFailPending = 0;
        packet.playerControlState = g_objectiveState.playerControlState;
        packet.movementLocked = g_objectiveState.movementLocked;
        packet.aimingLocked = g_objectiveState.aimingLocked;
        packet.firingLocked = g_objectiveState.firingLocked;
        packet.cameraLocked = g_objectiveState.cameraLocked;
        packet.hudHidden = g_objectiveState.hudHidden;
        packet.runtimeState = static_cast<uint8_t>(g_runtimeState);
        packet.terminalReasonCode = g_terminalReasonCode;
        packet.terminalSourceEventType = g_terminalSourceEventType;
        packet.terminalSourceOpcode = g_terminalSourceOpcode;
        packet.terminalSourceSequence = g_terminalSourceSequence;
        packet.terminalTieBreaker = terminalPriority(g_terminalReasonCode);
        return true;
    }

    void ApplyTerminalPending(CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        if (packet.passFailPending == 1)
        {
            CommitTerminalOutcome(packet, CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS);
        }
        else if (packet.passFailPending == 2)
        {
            CommitTerminalOutcome(packet, CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL);
        }
    }

    void ApplyCompletion()
    {
        g_runtimeState = RuntimeState::Teardown;
        g_isOnMission = false;
        g_hasCheckpoint = false;
        g_objectiveState.playerControlState = 1;
        g_objectiveState.movementLocked = 0;
        g_objectiveState.aimingLocked = 0;
        g_objectiveState.firingLocked = 0;
        g_objectiveState.cameraLocked = 0;
        g_objectiveState.hudHidden = 0;
    }

    void WriteTerminalMetadata(CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        switch (g_terminalReasonCode)
        {
        case CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL: packet.terminalTieBreaker = 3; break;
        case CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS: packet.terminalTieBreaker = 2; break;
        case CMissionRuntimePackets::MISSION_TERMINAL_REASON_ON_MISSION_CLEARED: packet.terminalTieBreaker = 1; break;
        default: packet.terminalTieBreaker = 0; break;
        }
        packet.terminalReasonCode = g_terminalReasonCode;
        packet.terminalSourceEventType = g_terminalSourceEventType;
        packet.terminalSourceOpcode = g_terminalSourceOpcode;
        packet.terminalSourceSequence = g_terminalSourceSequence;
    }

    MigrationSnapshot BuildMigrationSnapshot()
    {
        MigrationSnapshot snapshot{};
        snapshot.runtimeState = g_runtimeState;
        snapshot.lastFlow = g_lastFlow;
        snapshot.lastCheckpoint = g_lastCheckpoint;
        snapshot.hasFlow = g_hasFlow;
        snapshot.hasCheckpoint = g_hasCheckpoint;
        snapshot.isOnMission = g_isOnMission;
        snapshot.terminalEventSeen = g_terminalEventSeen;
        snapshot.terminalOutcomeCommitted = g_terminalOutcomeCommitted;
        snapshot.terminalReasonCode = g_terminalReasonCode;
        snapshot.terminalSourceEventType = g_terminalSourceEventType;
        snapshot.terminalSourceOpcode = g_terminalSourceOpcode;
        snapshot.terminalSourceSequence = g_terminalSourceSequence;
        snapshot.objectiveVersion = g_objectiveVersion;
        snapshot.checkpointVersion = g_checkpointVersion;
        snapshot.runtimeSessionToken = g_runtimeSessionToken;
        snapshot.lastSequence = g_lastSequence;
        snapshot.objectiveState = g_objectiveState;
        return snapshot;
    }

    void RestoreMigrationSnapshot(const MigrationSnapshot& snapshot)
    {
        g_runtimeState = snapshot.runtimeState;
        g_lastFlow = snapshot.lastFlow;
        g_lastCheckpoint = snapshot.lastCheckpoint;
        g_hasFlow = snapshot.hasFlow;
        g_hasCheckpoint = snapshot.hasCheckpoint;
        g_isOnMission = snapshot.isOnMission;
        g_terminalEventSeen = snapshot.terminalEventSeen;
        g_terminalOutcomeCommitted = snapshot.terminalOutcomeCommitted;
        g_terminalReasonCode = snapshot.terminalReasonCode;
        g_terminalSourceEventType = snapshot.terminalSourceEventType;
        g_terminalSourceOpcode = snapshot.terminalSourceOpcode;
        g_terminalSourceSequence = snapshot.terminalSourceSequence;
        g_objectiveVersion = snapshot.objectiveVersion;
        g_checkpointVersion = snapshot.checkpointVersion;
        g_runtimeSessionToken = snapshot.runtimeSessionToken;
        g_lastSequence = snapshot.lastSequence;
        g_objectiveState = snapshot.objectiveState;
    }

    bool CanAcceptEvent(CMissionRuntimeManager::EventKind eventKind, RuntimeState currentState)
    {
        switch (eventKind)
        {
        case CMissionRuntimeManager::EventKind::OnMissionFlagSync:
        case CMissionRuntimeManager::EventKind::MissionFlowSync:
            return true;
        case CMissionRuntimeManager::EventKind::CheckpointUpdate:
        case CMissionRuntimeManager::EventKind::CheckpointRemove:
            if (HasCommittedTerminalOutcome())
            {
                return false;
            }
            return currentState == RuntimeState::Starting
                || currentState == RuntimeState::Active
                || currentState == RuntimeState::PassPending
                || currentState == RuntimeState::FailPending;
        default:
            return false;
        }
    }

    bool ValidateCommonEventGuards(
        CMissionRuntimeManager::EventKind eventKind,
        CPlayer* sourcePlayer,
        const void* data,
        int size,
        int expectedSize)
    {
        if (!sourcePlayer || !sourcePlayer->m_bIsHost || !data || size < expectedSize)
        {
            return false;
        }

        if (g_missionAuthorityPlayerId != -1 && sourcePlayer->m_iPlayerId != g_missionAuthorityPlayerId)
        {
            return false;
        }

        return CanAcceptEvent(eventKind, g_runtimeState);
    }

    uint8_t ResolveTargetObjectiveTypeForOpcode(uint16_t opcode)
    {
        switch (opcode)
        {
        case 0x05E2:
        case 0x0634:
            return ObjectiveSync::State::TARGET_OBJECTIVE_KILL;
        case 0x0603:
            return ObjectiveSync::State::TARGET_OBJECTIVE_FLEE;
        case 0x05BF:
            return ObjectiveSync::State::TARGET_OBJECTIVE_PROTECT;
        default:
            return ObjectiveSync::State::TARGET_OBJECTIVE_NONE;
        }
    }

    void PromoteTargetLifecycle(uint8_t lifecycleState)
    {
        if (g_objectiveState.targetLifecycleState == lifecycleState)
        {
            return;
        }
        g_objectiveState.targetLifecycleState = lifecycleState;
        if (g_objectiveState.targetStateSequence < UINT16_MAX)
        {
            ++g_objectiveState.targetStateSequence;
        }
    }
}

void CMissionRuntimeManager::HandleOpcodeRegistrySync(const void* data, int size)
{
    if (!data || size < (int)sizeof(COpCodePackets::OpCodeSyncHeader))
    {
        return;
    }

    const auto* header = static_cast<const COpCodePackets::OpCodeSyncHeader*>(data);
    if (header->missionEpoch == 0 || header->scriptLocalIdentifier == 0 || header->missionEpoch != g_missionEpoch)
    {
        return;
    }

    const int maxParams = std::min<int>(header->intParamCount, 4);
    const auto* params = reinterpret_cast<const COpCodePackets::OpcodeParameter*>(
        static_cast<const uint8_t*>(data) + sizeof(COpCodePackets::OpCodeSyncHeader));

    for (int i = 0; i < maxParams; ++i)
    {
        if (params[i].entityId < 0)
        {
            continue;
        }

        if (params[i].entityType < 1 || params[i].entityType > 3)
        {
            continue;
        }

        RegistryActorKey key{};
        key.missionEpoch = header->missionEpoch;
        key.scriptLocalIdentifier = header->scriptLocalIdentifier;
        key.opcode = header->opcode;
        key.slot = static_cast<uint8_t>(i);
        key.entityType = static_cast<uint8_t>(params[i].entityType);

        CMissionRuntimePackets::MissionRuntimeSnapshotActor actor{};
        actor.actorType = key.entityType;
        actor.actorNetworkId = params[i].entityId;
        actor.roleFlags = 0;
        actor.isAlive = 1;
        actor.missionEpoch = key.missionEpoch;
        actor.scriptLocalIdentifier = key.scriptLocalIdentifier;
        actor.sourceOpcode = key.opcode;
        actor.sourceSlot = key.slot;
        g_registryActors[key] = actor;

        if (i == 1)
        {
            const uint8_t objectiveType = ResolveTargetObjectiveTypeForOpcode(header->opcode);
            if (objectiveType != ObjectiveSync::State::TARGET_OBJECTIVE_NONE)
            {
                g_objectiveState.targetObjectiveType = objectiveType;
                g_objectiveState.targetEntityType = key.entityType;
                g_objectiveState.targetEntityNetworkId = actor.actorNetworkId;
                PromoteTargetLifecycle(ObjectiveSync::State::TARGET_LIFECYCLE_SPAWN);
                PromoteTargetLifecycle(ObjectiveSync::State::TARGET_LIFECYCLE_ACTIVE);
            }
        }
    }

    g_snapshotActors.clear();
    g_snapshotActors.reserve(g_registryActors.size());
    for (const auto& [_, actor] : g_registryActors)
    {
        g_snapshotActors.push_back(actor);
    }
}

bool CMissionRuntimeManager::HandleOnMissionFlagSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!ValidateCommonEventGuards(EventKind::OnMissionFlagSync, sourcePlayer, data, size, (int)sizeof(OnMissionFlagPayload)))
    {
        return false;
    }

    auto* packet = (const OnMissionFlagPayload*)data;
    if (packet->missionEpoch != g_missionEpoch)
    {
        return false;
    }
    if (packet->sequenceId <= g_lastSequence)
    {
        return false;
    }

    const bool nextOnMission = packet->bOnMission != 0;

    if (nextOnMission)
    {
        if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Teardown)
        {
            ApplyMissionLaunch();
            if (g_runtimeSessionToken == 0)
            {
                g_runtimeSessionToken = 1;
            }
        }
        else if (g_isOnMission)
        {
            return false;
        }
        g_isOnMission = true;
    }
    else
    {
        if (!g_isOnMission)
        {
            return false;
        }

        if (g_isOnMission)
        {
            if (g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE)
            {
                g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_ON_MISSION_CLEARED;
                g_terminalSourceEventType = 0;
                g_terminalSourceOpcode = 0;
                g_terminalSourceSequence = g_lastSequence;
            }
            ApplyCompletion();
        }
    }

    g_lastSequence = packet->sequenceId;
    OnMissionFlagPayload outbound = *packet;
    outbound.missionEpoch = g_missionEpoch;
    outbound.sequenceId = g_lastSequence;
    CNetwork::SendPacketToAll(CPacketsID::ON_MISSION_FLAG_SYNC, &outbound, sizeof(outbound), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleMissionFlowSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!ValidateCommonEventGuards(EventKind::MissionFlowSync, sourcePlayer, data, size, (int)sizeof(MissionFlowPayload)))
    {
        return false;
    }

    auto* packet = (MissionFlowPayload*)data;
    if (packet->missionEpoch != g_missionEpoch)
    {
        return false;
    }

    if (packet->sequenceId <= g_lastSequence)
    {
        return false;
    }

    if (packet->onMission && !HasCommittedTerminalOutcome())
    {
        ApplyMissionLaunch();
    }

    if (!HasCommittedTerminalOutcome())
    {
        g_objectiveState.missionId = packet->missionId;
        g_objectiveState.timerMs = packet->timerMs;
        g_objectiveState.objectivePhaseIndex = packet->objectivePhaseIndex;
        g_objectiveState.checkpointIndex = packet->checkpointIndex;
        g_objectiveState.checkpointCount = packet->checkpointCount;
        g_objectiveState.timerVisible = packet->timerVisible;
        g_objectiveState.timerFrozen = packet->timerFrozen;
        g_objectiveState.timerDirection = packet->timerDirection;
        g_objectiveState.passFailPending = packet->passFailPending;
        g_objectiveState.playerControlState = packet->playerControlState;
        g_objectiveState.movementLocked = packet->movementLocked;
        g_objectiveState.aimingLocked = packet->aimingLocked;
        g_objectiveState.firingLocked = packet->firingLocked;
        g_objectiveState.cameraLocked = packet->cameraLocked;
        g_objectiveState.hudHidden = packet->hudHidden;
        g_objectiveState.cutscenePhase = packet->cutscenePhase;
        g_objectiveState.cutsceneSessionToken = packet->cutsceneSessionToken;
        g_objectiveState.objectiveTextToken = packet->objectiveTextToken;
        g_objectiveState.objectiveTextSemantics = packet->objectiveTextSemantics;
        g_objectiveState.respawnEligible = packet->respawnEligible;
        g_objectiveState.participantDeathCount = packet->participantDeathCount;
        g_objectiveState.participantIncapacitationCount = packet->participantIncapacitationCount;
        g_objectiveState.respawnCount = packet->respawnCount;
        g_objectiveState.missionFailThreshold = packet->missionFailThreshold == 0 ? 1 : packet->missionFailThreshold;
        g_objectiveState.incapacitationFailThreshold = packet->incapacitationFailThreshold;
        g_objectiveState.vehicleTaskState = packet->vehicleTaskState;
        g_objectiveState.pursuitState = packet->pursuitState;
        g_objectiveState.destroyEscapeState = packet->destroyEscapeState;
        g_objectiveState.vehicleTaskSequence = packet->vehicleTaskSequence;
        g_objectiveState.targetObjectiveType = packet->targetObjectiveType;
        g_objectiveState.targetLifecycleState = packet->targetLifecycleState;
        g_objectiveState.targetEntityNetworkId = packet->targetEntityNetworkId;
        g_objectiveState.targetEntityType = packet->targetEntityType;
        g_objectiveState.targetStateSequence = packet->targetStateSequence;
        g_objectiveState.stealthState = packet->stealthState;
        g_objectiveState.detectionSourceMask = packet->detectionSourceMask;
        g_objectiveState.stealthStateSequence = packet->stealthStateSequence;
        g_objectiveState.objectiveModifierFlags = packet->objectiveModifierFlags;
        std::memcpy(g_objectiveState.objective, packet->objective, sizeof(g_objectiveState.objective));

        if (packet->sourceOpcode == 0x0417 || (packet->missionId != 0 && packet->missionId != g_lastFlow.missionId))
        {
            const MissionRuntimePolicy policy = ResolveMissionRuntimePolicy(packet->missionId);
            if (packet->missionFailThreshold == 0)
            {
                g_objectiveState.missionFailThreshold = policy.missionFailThreshold;
                packet->missionFailThreshold = policy.missionFailThreshold;
            }
            if (packet->incapacitationFailThreshold == 0)
            {
                g_objectiveState.incapacitationFailThreshold = policy.incapacitationFailThreshold;
                packet->incapacitationFailThreshold = policy.incapacitationFailThreshold;
            }
        }

        if (packet->eventType == MISSION_FLOW_EVENT_PARTICIPANT_DEATH
            || packet->eventType == MISSION_FLOW_EVENT_PARTICIPANT_INCAPACITATED)
        {
            if (packet->eventType == MISSION_FLOW_EVENT_PARTICIPANT_DEATH)
            {
                PromoteTargetLifecycle(ObjectiveSync::State::TARGET_LIFECYCLE_DEAD);
            }
            else
            {
                PromoteTargetLifecycle(ObjectiveSync::State::TARGET_LIFECYCLE_DOWNED);
            }
            const bool deathThresholdHit = g_objectiveState.missionFailThreshold > 0
                && g_objectiveState.participantDeathCount >= g_objectiveState.missionFailThreshold;
            const bool incapThresholdHit = g_objectiveState.incapacitationFailThreshold != UINT8_MAX
                && g_objectiveState.participantIncapacitationCount >= g_objectiveState.incapacitationFailThreshold;
            if ((deathThresholdHit || incapThresholdHit) && packet->passFailPending == 0)
            {
                packet->passFailPending = 2;
            }
        }
        if (g_objectiveState.destroyEscapeState == ObjectiveSync::State::DESTROY_ESCAPE_STATE_ESCAPED
            || g_objectiveState.pursuitState == ObjectiveSync::State::PURSUIT_STATE_ESCAPING)
        {
            PromoteTargetLifecycle(ObjectiveSync::State::TARGET_LIFECYCLE_ESCAPED);
        }
        ApplyStealthFromEvent(*packet);

        if (g_hasFlow)
        {
            ApplyObjectiveUpdate(*packet);
            ApplyCheckpointProgression(*packet);
        }

        ApplyTerminalPending(*packet);

        if (!packet->onMission && g_isOnMission)
        {
            if (g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE)
            {
                g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_ON_MISSION_CLEARED;
                g_terminalSourceEventType = packet->eventType;
                g_terminalSourceOpcode = packet->sourceOpcode;
                g_terminalSourceSequence = packet->sequence;
            }
            ApplyCompletion();
        }
    }
    g_isOnMission = packet->onMission != 0;

    const uint16_t previousMissionId = g_hasFlow ? g_lastFlow.missionId : 0;
    g_lastSequence = packet->sequenceId;
    g_lastFlow = *packet;
    g_lastFlow.replay = 0;
    g_lastFlow.runtimeState = static_cast<uint8_t>(g_runtimeState);
    g_lastFlow.objectiveVersion = g_objectiveVersion;
    g_lastFlow.checkpointVersion = g_checkpointVersion;
    g_lastFlow.missionEpoch = g_missionEpoch;
    g_lastFlow.sequenceId = g_lastSequence;
    WriteTerminalMetadata(g_lastFlow);

    if (g_runtimeSessionToken == 0 || (g_hasFlow && packet->missionId != previousMissionId))
    {
        ++g_runtimeSessionToken;
    }
    g_lastFlow.runtimeSessionToken = g_runtimeSessionToken;

    packet->replay = 0;
    packet->runtimeState = g_lastFlow.runtimeState;
    packet->objectiveVersion = g_objectiveVersion;
    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    packet->missionEpoch = g_missionEpoch;
    packet->sequenceId = g_lastSequence;
    packet->respawnEligible = g_objectiveState.respawnEligible;
    packet->participantDeathCount = g_objectiveState.participantDeathCount;
    packet->participantIncapacitationCount = g_objectiveState.participantIncapacitationCount;
    packet->respawnCount = g_objectiveState.respawnCount;
    packet->missionFailThreshold = g_objectiveState.missionFailThreshold;
    packet->incapacitationFailThreshold = g_objectiveState.incapacitationFailThreshold;
    packet->vehicleTaskState = g_objectiveState.vehicleTaskState;
    packet->pursuitState = g_objectiveState.pursuitState;
    packet->destroyEscapeState = g_objectiveState.destroyEscapeState;
    packet->vehicleTaskSequence = g_objectiveState.vehicleTaskSequence;
    packet->targetObjectiveType = g_objectiveState.targetObjectiveType;
    packet->targetLifecycleState = g_objectiveState.targetLifecycleState;
    packet->targetEntityNetworkId = g_objectiveState.targetEntityNetworkId;
    packet->targetEntityType = g_objectiveState.targetEntityType;
    packet->targetStateSequence = g_objectiveState.targetStateSequence;
    packet->stealthState = g_objectiveState.stealthState;
    packet->detectionSourceMask = g_objectiveState.detectionSourceMask;
    packet->stealthStateSequence = g_objectiveState.stealthStateSequence;
    packet->objectiveModifierFlags = g_objectiveState.objectiveModifierFlags;
    WriteTerminalMetadata(*packet);

    g_hasFlow = true;
    CNetwork::SendPacketToAll(CPacketsID::MISSION_FLOW_SYNC, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
    return true;
}

bool CMissionRuntimeManager::HandleCheckpointUpdate(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!ValidateCommonEventGuards(EventKind::CheckpointUpdate, sourcePlayer, data, size, (int)sizeof(CheckpointPayload)))
    {
        return false;
    }

    auto* packet = (CheckpointPayload*)data;
    if (packet->missionEpoch != g_missionEpoch)
    {
        return false;
    }
    if (packet->sequenceId <= g_lastSequence)
    {
        return false;
    }

    if (packet->checkpointIndex < g_lastCheckpoint.checkpointIndex)
    {
        return false;
    }

    ++g_checkpointVersion;
    g_lastCheckpoint = *packet;
    g_lastCheckpoint.checkpointVersion = g_checkpointVersion;
    g_lastCheckpoint.runtimeSessionToken = g_runtimeSessionToken;
    g_lastCheckpoint.missionEpoch = g_missionEpoch;
    g_lastCheckpoint.sequenceId = packet->sequenceId;
    g_lastSequence = packet->sequenceId;
    g_hasCheckpoint = true;

    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    packet->missionEpoch = g_missionEpoch;
    packet->sequenceId = g_lastSequence;
    CNetwork::SendPacketToAll(CPacketsID::UPDATE_CHECKPOINT, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleCheckpointRemove(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!ValidateCommonEventGuards(EventKind::CheckpointRemove, sourcePlayer, data, size, (int)sizeof(CheckpointRemovePayload)))
    {
        return false;
    }

    auto* packet = (CheckpointRemovePayload*)data;
    if (packet->missionEpoch != g_missionEpoch)
    {
        return false;
    }
    if (packet->sequenceId <= g_lastSequence)
    {
        return false;
    }

    ++g_checkpointVersion;
    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    packet->missionEpoch = g_missionEpoch;
    g_lastSequence = packet->sequenceId;
    packet->sequenceId = g_lastSequence;
    g_hasCheckpoint = false;

    CNetwork::SendPacketToAll(CPacketsID::REMOVE_CHECKPOINT, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleMissionEvent(EventKind eventKind, CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    switch (eventKind)
    {
    case EventKind::OnMissionFlagSync:
        return HandleOnMissionFlagSync(sourcePlayer, sourcePeer, data, size);
    case EventKind::MissionFlowSync:
        return HandleMissionFlowSync(sourcePlayer, sourcePeer, data, size);
    case EventKind::CheckpointUpdate:
        return HandleCheckpointUpdate(sourcePlayer, sourcePeer, data, size);
    case EventKind::CheckpointRemove:
        return HandleCheckpointRemove(sourcePlayer, sourcePeer, data, size);
    default:
        return false;
    }
}

void CMissionRuntimeManager::SendSnapshotTo(ENetPeer* peer)
{
    if (!peer)
    {
        return;
    }

    ++g_snapshotVersion;
    CMissionRuntimePackets::MissionRuntimeSnapshotBegin snapshotBegin{};
    snapshotBegin.snapshotVersion = g_snapshotVersion;
    snapshotBegin.actorCount = static_cast<uint8_t>(std::min<size_t>(g_snapshotActors.size(), UINT8_MAX));
    CNetwork::SendPacket(peer, CPacketsID::MISSION_RUNTIME_SNAPSHOT_BEGIN, &snapshotBegin, sizeof(snapshotBegin), ENET_PACKET_FLAG_RELIABLE);

    CMissionRuntimePackets::MissionRuntimeSnapshotState snapshotState{};
    snapshotState.runtimeState = static_cast<uint8_t>(g_runtimeState);
    snapshotState.onMission = g_isOnMission ? 1 : 0;
    snapshotState.missionId = g_objectiveState.missionId;
    snapshotState.objectivePhaseIndex = g_objectiveState.objectivePhaseIndex;
    snapshotState.objectiveTextToken = g_objectiveState.objectiveTextToken;
    snapshotState.objectiveTextSemantics = g_objectiveState.objectiveTextSemantics;
    std::memcpy(snapshotState.objective, g_objectiveState.objective, sizeof(snapshotState.objective));
    snapshotState.timerMs = g_objectiveState.timerMs;
    snapshotState.timerVisible = g_objectiveState.timerVisible;
    snapshotState.timerFrozen = g_objectiveState.timerFrozen;
    snapshotState.timerDirection = g_objectiveState.timerDirection;
    snapshotState.checkpointIndex = g_objectiveState.checkpointIndex;
    snapshotState.checkpointCount = g_objectiveState.checkpointCount;
    snapshotState.playerControlState = g_objectiveState.playerControlState;
    snapshotState.movementLocked = g_objectiveState.movementLocked;
    snapshotState.aimingLocked = g_objectiveState.aimingLocked;
    snapshotState.firingLocked = g_objectiveState.firingLocked;
    snapshotState.cameraLocked = g_objectiveState.cameraLocked;
    snapshotState.hudHidden = g_objectiveState.hudHidden;
    snapshotState.objectiveVersion = g_objectiveVersion;
    snapshotState.checkpointVersion = g_checkpointVersion;
    snapshotState.runtimeSessionToken = g_runtimeSessionToken;
    snapshotState.missionEpoch = g_missionEpoch;
    snapshotState.sequenceId = g_lastSequence;
    snapshotState.terminalReasonCode = g_terminalReasonCode;
    snapshotState.terminalSourceEventType = g_terminalSourceEventType;
    snapshotState.terminalSourceOpcode = g_terminalSourceOpcode;
    snapshotState.terminalSourceSequence = g_terminalSourceSequence;
    snapshotState.passFailPending = g_lastFlow.passFailPending;
    snapshotState.cutscenePhase = g_objectiveState.cutscenePhase;
    snapshotState.cutsceneSessionToken = g_objectiveState.cutsceneSessionToken;
    snapshotState.respawnEligible = g_objectiveState.respawnEligible;
    snapshotState.participantDeathCount = g_objectiveState.participantDeathCount;
    snapshotState.participantIncapacitationCount = g_objectiveState.participantIncapacitationCount;
    snapshotState.respawnCount = g_objectiveState.respawnCount;
    snapshotState.missionFailThreshold = g_objectiveState.missionFailThreshold;
    snapshotState.incapacitationFailThreshold = g_objectiveState.incapacitationFailThreshold;
    snapshotState.vehicleTaskState = g_objectiveState.vehicleTaskState;
    snapshotState.pursuitState = g_objectiveState.pursuitState;
    snapshotState.destroyEscapeState = g_objectiveState.destroyEscapeState;
    snapshotState.vehicleTaskSequence = g_objectiveState.vehicleTaskSequence;
    snapshotState.targetObjectiveType = g_objectiveState.targetObjectiveType;
    snapshotState.targetLifecycleState = g_objectiveState.targetLifecycleState;
    snapshotState.targetEntityNetworkId = g_objectiveState.targetEntityNetworkId;
    snapshotState.targetEntityType = g_objectiveState.targetEntityType;
    snapshotState.targetStateSequence = g_objectiveState.targetStateSequence;
    snapshotState.stealthState = g_objectiveState.stealthState;
    snapshotState.detectionSourceMask = g_objectiveState.detectionSourceMask;
    snapshotState.stealthStateSequence = g_objectiveState.stealthStateSequence;
    snapshotState.objectiveModifierFlags = g_objectiveState.objectiveModifierFlags;
    snapshotState.terminalTieBreaker = g_lastFlow.terminalTieBreaker;
    CNetwork::SendPacket(peer, CPacketsID::MISSION_RUNTIME_SNAPSHOT_STATE, &snapshotState, sizeof(snapshotState), ENET_PACKET_FLAG_RELIABLE);

    for (size_t i = 0; i < std::min<size_t>(g_snapshotActors.size(), UINT8_MAX); ++i)
    {
        CNetwork::SendPacket(peer, CPacketsID::MISSION_RUNTIME_SNAPSHOT_ACTOR, &g_snapshotActors[i], sizeof(g_snapshotActors[i]), ENET_PACKET_FLAG_RELIABLE);
    }

    CMissionRuntimePackets::MissionRuntimeSnapshotEnd snapshotEnd{};
    snapshotEnd.snapshotVersion = g_snapshotVersion;
    CNetwork::SendPacket(peer, CPacketsID::MISSION_RUNTIME_SNAPSHOT_END, &snapshotEnd, sizeof(snapshotEnd), ENET_PACKET_FLAG_RELIABLE);

    OnMissionFlagPayload onMissionPacket{};
    onMissionPacket.bOnMission = g_isOnMission ? 1 : 0;
    onMissionPacket.missionEpoch = g_missionEpoch;
    onMissionPacket.sequenceId = g_lastSequence;
    CNetwork::SendPacket(peer, CPacketsID::ON_MISSION_FLAG_SYNC, &onMissionPacket, sizeof(onMissionPacket), ENET_PACKET_FLAG_RELIABLE);

    if (g_hasFlow)
    {
        MissionFlowPayload replayPacket = g_lastFlow;
        replayPacket.replay = 1;
        replayPacket.runtimeState = static_cast<uint8_t>(g_runtimeState);
        replayPacket.objectiveVersion = g_objectiveVersion;
        replayPacket.checkpointVersion = g_checkpointVersion;
        replayPacket.runtimeSessionToken = g_runtimeSessionToken;
        replayPacket.missionEpoch = g_missionEpoch;
        replayPacket.sequenceId = g_lastSequence;
        WriteTerminalMetadata(replayPacket);
        CNetwork::SendPacket(peer, CPacketsID::MISSION_FLOW_SYNC, &replayPacket, sizeof(replayPacket), ENET_PACKET_FLAG_RELIABLE);
    }

    if (g_hasCheckpoint && (g_runtimeState == RuntimeState::Active || g_runtimeState == RuntimeState::PassPending || g_runtimeState == RuntimeState::FailPending))
    {
        CheckpointPayload checkpointPacket = g_lastCheckpoint;
        checkpointPacket.checkpointVersion = g_checkpointVersion;
        checkpointPacket.runtimeSessionToken = g_runtimeSessionToken;
        checkpointPacket.missionEpoch = g_missionEpoch;
        checkpointPacket.sequenceId = g_lastSequence;
        CNetwork::SendPacket(peer, CPacketsID::UPDATE_CHECKPOINT, &checkpointPacket, sizeof(checkpointPacket), ENET_PACKET_FLAG_RELIABLE);
    }
}

uint32_t CMissionRuntimeManager::HandleHostMigration(int newHostPlayerId)
{
    const MigrationSnapshot snapshot = BuildMigrationSnapshot();
    ++g_missionEpoch;
    g_missionAuthorityPlayerId = newHostPlayerId;
    g_lastSequence = snapshot.lastSequence;

    if (snapshot.hasFlow || snapshot.hasCheckpoint || snapshot.isOnMission)
    {
        RestoreMigrationSnapshot(snapshot);
        if (g_hasFlow)
        {
            g_lastFlow.sequence = g_lastSequence;
            g_lastFlow.runtimeState = static_cast<uint8_t>(g_runtimeState);
            g_lastFlow.objectiveVersion = g_objectiveVersion;
            g_lastFlow.checkpointVersion = g_checkpointVersion;
            g_lastFlow.runtimeSessionToken = g_runtimeSessionToken;
            g_lastFlow.missionEpoch = g_missionEpoch;
            g_lastFlow.sequenceId = g_lastSequence;
            WriteTerminalMetadata(g_lastFlow);
        }
        if (g_hasCheckpoint)
        {
            g_lastCheckpoint.missionEpoch = g_missionEpoch;
            g_lastCheckpoint.sequenceId = g_lastSequence;
        }
    }

    if (!g_registryActors.empty())
    {
        std::unordered_map<RegistryActorKey, CMissionRuntimePackets::MissionRuntimeSnapshotActor, RegistryActorKeyHash> remapped{};
        remapped.reserve(g_registryActors.size());
        for (auto& [key, actor] : g_registryActors)
        {
            key.missionEpoch = g_missionEpoch;
            actor.missionEpoch = g_missionEpoch;
            remapped[key] = actor;
        }
        g_registryActors = std::move(remapped);
    }

    g_snapshotActors.clear();
    g_snapshotActors.reserve(g_registryActors.size());
    for (const auto& [_, actor] : g_registryActors)
    {
        g_snapshotActors.push_back(actor);
    }

    return g_missionEpoch;
}

uint32_t CMissionRuntimeManager::GetMissionEpoch()
{
    return g_missionEpoch;
}

uint32_t CMissionRuntimeManager::GetLastSequenceId()
{
    return g_lastSequence;
}

void CMissionRuntimeManager::Teardown()
{
    g_runtimeState = RuntimeState::Idle;
    g_lastFlow = {};
    g_lastCheckpoint = {};
    g_hasFlow = false;
    g_hasCheckpoint = false;
    g_isOnMission = false;
    g_terminalEventSeen = false;
    g_terminalOutcomeCommitted = false;
    g_lastSequence = 0;
    g_objectiveVersion = 0;
    g_checkpointVersion = 0;
    ++g_runtimeSessionToken;
    ++g_missionEpoch;
    g_missionAuthorityPlayerId = -1;
    g_objectiveState = {};
    g_snapshotActors.clear();
    g_registryActors.clear();
    g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
    g_terminalSourceEventType = 0;
    g_terminalSourceOpcode = 0;
    g_terminalSourceSequence = 0;
}
