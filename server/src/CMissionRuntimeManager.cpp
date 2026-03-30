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
    uint32_t g_lastSequence = 0;
    uint16_t g_objectiveVersion = 0;
    uint16_t g_checkpointVersion = 0;
    uint32_t g_runtimeSessionToken = 0;
    ObjectiveSync::State g_objectiveState{};

    bool IsTerminal(RuntimeState state)
    {
        return state == RuntimeState::Completed;
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
            if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Completed)
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
        case RuntimeState::Completed:
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
        if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Completed)
        {
            Transition(RuntimeState::Starting);
            Transition(RuntimeState::Active);
            g_terminalEventSeen = false;
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

    void ApplyTerminalPending(uint8_t pending)
    {
        if (pending == 1)
        {
            Transition(RuntimeState::PassPending);
        }
        else if (pending == 2)
        {
            Transition(RuntimeState::FailPending);
        }
    }

    void ApplyCompletion()
    {
        Transition(RuntimeState::Completed);
        g_isOnMission = false;
    }
}

bool CMissionRuntimeManager::HandleOnMissionFlagSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!sourcePlayer || !sourcePlayer->m_bIsHost || !data || size < (int)sizeof(OnMissionFlagPayload))
    {
        return false;
    }

    auto* packet = (const OnMissionFlagPayload*)data;
    const bool nextOnMission = packet->bOnMission != 0;

    if (nextOnMission)
    {
        if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Completed)
        {
            ApplyMissionLaunch();
            if (g_runtimeSessionToken == 0)
            {
                g_runtimeSessionToken = 1;
            }
        }
        g_isOnMission = true;
    }
    else
    {
        if (g_isOnMission)
        {
            ApplyCompletion();
        }
    }

    CNetwork::SendPacketToAll(CPacketsID::ON_MISSION_FLAG_SYNC, (void*)packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleMissionFlowSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!sourcePlayer || !sourcePlayer->m_bIsHost || !data || size < (int)sizeof(MissionFlowPayload))
    {
        return false;
    }

    auto* packet = (MissionFlowPayload*)data;
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

    ApplyTerminalPending(packet->passFailPending);

    if (!packet->onMission && g_isOnMission)
    {
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

    g_hasFlow = true;
    CNetwork::SendPacketToAll(CPacketsID::MISSION_FLOW_SYNC, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleCheckpointUpdate(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!sourcePlayer || !sourcePlayer->m_bIsHost || !data || size < (int)sizeof(CheckpointPayload))
    {
        return false;
    }

    if (g_runtimeState == RuntimeState::Idle || g_runtimeState == RuntimeState::Completed)
    {
        return false;
    }

    auto* packet = (CheckpointPayload*)data;
    if (packet->checkpointIndex < g_lastCheckpoint.checkpointIndex)
    {
        return false;
    }

    ++g_checkpointVersion;
    g_lastCheckpoint = *packet;
    g_lastCheckpoint.checkpointVersion = g_checkpointVersion;
    g_lastCheckpoint.runtimeSessionToken = g_runtimeSessionToken;
    g_hasCheckpoint = true;

    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    CNetwork::SendPacketToAll(CPacketsID::UPDATE_CHECKPOINT, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

bool CMissionRuntimeManager::HandleCheckpointRemove(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size)
{
    if (!sourcePlayer || !sourcePlayer->m_bIsHost || !data || size < (int)sizeof(CheckpointRemovePayload))
    {
        return false;
    }

    auto* packet = (CheckpointRemovePayload*)data;
    ++g_checkpointVersion;
    packet->checkpointVersion = g_checkpointVersion;
    packet->runtimeSessionToken = g_runtimeSessionToken;
    g_hasCheckpoint = false;

    CNetwork::SendPacketToAll(CPacketsID::REMOVE_CHECKPOINT, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, sourcePeer);
    return true;
}

void CMissionRuntimeManager::SendSnapshotTo(ENetPeer* peer)
{
    if (!peer)
    {
        return;
    }

    OnMissionFlagPayload onMissionPacket{};
    onMissionPacket.bOnMission = g_isOnMission ? 1 : 0;
    CNetwork::SendPacket(peer, CPacketsID::ON_MISSION_FLAG_SYNC, &onMissionPacket, sizeof(onMissionPacket), ENET_PACKET_FLAG_RELIABLE);

    if (g_hasFlow)
    {
        MissionFlowPayload replayPacket = g_lastFlow;
        replayPacket.replay = 1;
        replayPacket.runtimeState = static_cast<uint8_t>(g_runtimeState);
        replayPacket.objectiveVersion = g_objectiveVersion;
        replayPacket.checkpointVersion = g_checkpointVersion;
        replayPacket.runtimeSessionToken = g_runtimeSessionToken;
        CNetwork::SendPacket(peer, CPacketsID::MISSION_FLOW_SYNC, &replayPacket, sizeof(replayPacket), ENET_PACKET_FLAG_RELIABLE);
    }

    if (g_hasCheckpoint && (g_runtimeState == RuntimeState::Active || g_runtimeState == RuntimeState::PassPending || g_runtimeState == RuntimeState::FailPending))
    {
        CheckpointPayload checkpointPacket = g_lastCheckpoint;
        checkpointPacket.checkpointVersion = g_checkpointVersion;
        checkpointPacket.runtimeSessionToken = g_runtimeSessionToken;
        CNetwork::SendPacket(peer, CPacketsID::UPDATE_CHECKPOINT, &checkpointPacket, sizeof(checkpointPacket), ENET_PACKET_FLAG_RELIABLE);
    }
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
    g_objectiveState = {};
}
