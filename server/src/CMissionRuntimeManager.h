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
        Completed = 5,
    };

    using OnMissionFlagPayload = CMissionRuntimePackets::OnMissionFlagSync;
    using MissionFlowPayload = CMissionRuntimePackets::MissionFlowSync;
    using CheckpointPayload = CMissionRuntimePackets::UpdateCheckpoint;
    using CheckpointRemovePayload = CMissionRuntimePackets::RemoveCheckpoint;

    static bool HandleOnMissionFlagSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleMissionFlowSync(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleCheckpointUpdate(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static bool HandleCheckpointRemove(CPlayer* sourcePlayer, ENetPeer* sourcePeer, const void* data, int size);
    static void SendSnapshotTo(ENetPeer* peer);
    static void Teardown();
};
