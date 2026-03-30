#include "CMissionRuntimeManager.h"

#include <algorithm>
#include <cstring>

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
    uint8_t g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
    uint8_t g_terminalSourceEventType = 0;
    uint16_t g_terminalSourceOpcode = 0;
    uint32_t g_terminalSourceSequence = 0;
    uint32_t g_lastSequence = 0;
    uint16_t g_objectiveVersion = 0;
    uint16_t g_checkpointVersion = 0;
    uint32_t g_runtimeSessionToken = 0;
    uint32_t g_missionEpoch = 1;
    int g_missionAuthorityPlayerId = -1;
    ObjectiveSync::State g_objectiveState{};
    
    struct MigrationSnapshot
    {
        RuntimeState runtimeState = RuntimeState::Idle;
        CMissionRuntimeManager::MissionFlowPayload lastFlow{};
        CMissionRuntimeManager::CheckpointPayload lastCheckpoint{};
        bool hasFlow = false;
        bool hasCheckpoint = false;
        bool isOnMission = false;
        bool terminalEventSeen = false;
        uint8_t terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
        uint8_t terminalSourceEventType = 0;
        uint16_t terminalSourceOpcode = 0;
        uint32_t terminalSourceSequence = 0;
        uint16_t objectiveVersion = 0;
        uint16_t checkpointVersion = 0;
        uint32_t runtimeSessionToken = 0;
        ObjectiveSync::State objectiveState{};
    };

    bool IsTerminal(RuntimeState state)
    {
        return state == RuntimeState::Teardown;
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
            || g_lastFlow.objectivePhaseIndex != packet.objectivePhaseIndex)
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

    void ApplyTerminalPending(const CMissionRuntimeManager::MissionFlowPayload& packet)
    {
        if (packet.passFailPending == 1)
        {
            if (Transition(RuntimeState::PassPending))
            {
                g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_PASS;
                g_terminalSourceEventType = packet.eventType;
                g_terminalSourceOpcode = packet.sourceOpcode;
                g_terminalSourceSequence = packet.sequence;
            }
        }
        else if (packet.passFailPending == 2)
        {
            if (Transition(RuntimeState::FailPending))
            {
                g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_FAIL;
                g_terminalSourceEventType = packet.eventType;
                g_terminalSourceOpcode = packet.sourceOpcode;
                g_terminalSourceSequence = packet.sequence;
            }
        }
    }

    void ApplyCompletion()
    {
        Transition(RuntimeState::Teardown);
        g_isOnMission = false;
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
        snapshot.terminalReasonCode = g_terminalReasonCode;
        snapshot.terminalSourceEventType = g_terminalSourceEventType;
        snapshot.terminalSourceOpcode = g_terminalSourceOpcode;
        snapshot.terminalSourceSequence = g_terminalSourceSequence;
        snapshot.objectiveVersion = g_objectiveVersion;
        snapshot.checkpointVersion = g_checkpointVersion;
        snapshot.runtimeSessionToken = g_runtimeSessionToken;
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
        g_terminalReasonCode = snapshot.terminalReasonCode;
        g_terminalSourceEventType = snapshot.terminalSourceEventType;
        g_terminalSourceOpcode = snapshot.terminalSourceOpcode;
        g_terminalSourceSequence = snapshot.terminalSourceSequence;
        g_objectiveVersion = snapshot.objectiveVersion;
        g_checkpointVersion = snapshot.checkpointVersion;
        g_runtimeSessionToken = snapshot.runtimeSessionToken;
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

    CNetwork::SendPacketToAll(CPacketsID::ON_MISSION_FLAG_SYNC, (void*)packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
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

    if (packet->sequence <= g_lastSequence)
    {
        return false;
    }

    if (packet->onMission)
    {
        ApplyMissionLaunch();
    }

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
    g_isOnMission = packet->onMission != 0;

    const uint16_t previousMissionId = g_hasFlow ? g_lastFlow.missionId : 0;
    g_lastSequence = packet->sequence;
    g_lastFlow = *packet;
    g_lastFlow.replay = 0;
    g_lastFlow.runtimeState = static_cast<uint8_t>(g_runtimeState);
    g_lastFlow.objectiveVersion = g_objectiveVersion;
    g_lastFlow.checkpointVersion = g_checkpointVersion;
    g_lastFlow.missionEpoch = g_missionEpoch;
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

    if (packet->checkpointIndex < g_lastCheckpoint.checkpointIndex)
    {
        return false;
    }

    ++g_checkpointVersion;
    g_lastCheckpoint = *packet;
    g_lastCheckpoint.checkpointVersion = g_checkpointVersion;
    g_lastCheckpoint.runtimeSessionToken = g_runtimeSessionToken;
    g_lastCheckpoint.missionEpoch = g_missionEpoch;
    g_hasCheckpoint = true;

    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    packet->missionEpoch = g_missionEpoch;
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

    ++g_checkpointVersion;
    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    packet->missionEpoch = g_missionEpoch;
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

    OnMissionFlagPayload onMissionPacket{};
    onMissionPacket.bOnMission = g_isOnMission ? 1 : 0;
    onMissionPacket.missionEpoch = g_missionEpoch;
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
        WriteTerminalMetadata(replayPacket);
        CNetwork::SendPacket(peer, CPacketsID::MISSION_FLOW_SYNC, &replayPacket, sizeof(replayPacket), ENET_PACKET_FLAG_RELIABLE);
    }

    if (g_hasCheckpoint && (g_runtimeState == RuntimeState::Active || g_runtimeState == RuntimeState::PassPending || g_runtimeState == RuntimeState::FailPending))
    {
        CheckpointPayload checkpointPacket = g_lastCheckpoint;
        checkpointPacket.checkpointVersion = g_checkpointVersion;
        checkpointPacket.runtimeSessionToken = g_runtimeSessionToken;
        checkpointPacket.missionEpoch = g_missionEpoch;
        CNetwork::SendPacket(peer, CPacketsID::UPDATE_CHECKPOINT, &checkpointPacket, sizeof(checkpointPacket), ENET_PACKET_FLAG_RELIABLE);
    }
}

uint32_t CMissionRuntimeManager::HandleHostMigration(int newHostPlayerId)
{
    const MigrationSnapshot snapshot = BuildMigrationSnapshot();
    ++g_missionEpoch;
    g_missionAuthorityPlayerId = newHostPlayerId;
    g_lastSequence = 0;

    if (snapshot.hasFlow || snapshot.hasCheckpoint || snapshot.isOnMission)
    {
        RestoreMigrationSnapshot(snapshot);
        g_lastSequence = 0;
        if (g_hasFlow)
        {
            g_lastFlow.sequence = 0;
            g_lastFlow.runtimeState = static_cast<uint8_t>(g_runtimeState);
            g_lastFlow.objectiveVersion = g_objectiveVersion;
            g_lastFlow.checkpointVersion = g_checkpointVersion;
            g_lastFlow.runtimeSessionToken = g_runtimeSessionToken;
            g_lastFlow.missionEpoch = g_missionEpoch;
            WriteTerminalMetadata(g_lastFlow);
        }
    }

    return g_missionEpoch;
}

uint32_t CMissionRuntimeManager::GetMissionEpoch()
{
    return g_missionEpoch;
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
    g_lastSequence = 0;
    g_objectiveVersion = 0;
    g_checkpointVersion = 0;
    ++g_runtimeSessionToken;
    ++g_missionEpoch;
    g_missionAuthorityPlayerId = -1;
    g_objectiveState = {};
    g_terminalReasonCode = CMissionRuntimePackets::MISSION_TERMINAL_REASON_NONE;
    g_terminalSourceEventType = 0;
    g_terminalSourceOpcode = 0;
    g_terminalSourceSequence = 0;
}
