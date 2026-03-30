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

    bool HasCommittedTerminalOutcome()
    {
        return g_terminalOutcomeCommitted
            && (g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS
                || g_terminalReasonCode == CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL);
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
        if (HasCommittedTerminalOutcome())
        {
            return false;
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

        packet.onMission = 0;
        packet.passFailPending = 0;
        packet.runtimeState = static_cast<uint8_t>(g_runtimeState);
        packet.terminalReasonCode = g_terminalReasonCode;
        packet.terminalSourceEventType = g_terminalSourceEventType;
        packet.terminalSourceOpcode = g_terminalSourceOpcode;
        packet.terminalSourceSequence = g_terminalSourceSequence;
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
    }

    void WriteTerminalMetadata(CMissionRuntimeManager::MissionFlowPayload& packet)
    {
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
        g_objectiveState.firingLocked = packet->firingLocked;
        g_objectiveState.cameraLocked = packet->cameraLocked;
        g_objectiveState.hudHidden = packet->hudHidden;
        g_objectiveState.cutscenePhase = packet->cutscenePhase;
        g_objectiveState.cutsceneSessionToken = packet->cutsceneSessionToken;
        g_objectiveState.objectiveTextToken = packet->objectiveTextToken;
        g_objectiveState.objectiveTextSemantics = packet->objectiveTextSemantics;
        std::memcpy(g_objectiveState.objective, packet->objective, sizeof(g_objectiveState.objective));

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
