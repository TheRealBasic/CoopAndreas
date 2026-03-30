#pragma once

#include <cstdint>
#include "CPacket.h"
#include "enet/enet.h"

class CPlayer;

class CMissionRuntimeManager
{
public:
    enum class RuntimeState : uint8_t
    {
        Idle = 0,
        Starting = 1,
        Active = 2,
        PassPending = 3,
        FailPending = 4,
        Teardown = 5,
    };

    enum class EventKind : uint8_t
    {
        OnMissionFlagSync,
        MissionFlowSync,
        CheckpointUpdate,
        CheckpointRemove,
    };

    using OnMissionFlagPayload = CMissionRuntimePackets::OnMissionFlagSync;
    using MissionFlowPayload = CMissionRuntimePackets::MissionFlowSync;
    using CheckpointPayload = CMissionRuntimePackets::UpdateCheckpoint;
    using CheckpointRemovePayload = CMissionRuntimePackets::RemoveCheckpoint;

    static bool HandleOnMissionFlagSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleMissionFlowSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleCheckpointUpdate(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleCheckpointRemove(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleMissionEvent(EventKind eventKind, CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static void SendSnapshotTo(ENetPeer* peer);
    static uint32_t HandleHostMigration(int newHostPlayerId);
    static uint32_t GetMissionEpoch();
    static uint32_t GetLastSequenceId();
    static void Teardown();
};
