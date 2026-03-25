#include "stdafx.h"
#include "CNetworkFireManager.h"

std::unordered_map<uint32_t, CNetworkFireManager::FireEntity> CNetworkFireManager::ms_fires;

int CNetworkFireManager::StartScriptFire(const CPackets::FireCreate& packet)
{
    int handle = -1;
    const int propagation = 1;
    const int size = std::clamp(static_cast<int>(packet.radius), 1, 8);
    plugin::Command<0x02CF>(packet.position.x, packet.position.y, packet.position.z, propagation, size, &handle);
    return handle;
}

void CNetworkFireManager::RemoveScriptFireSafe(int scriptHandle)
{
    if (scriptHandle < 0)
    {
        return;
    }

    plugin::Command<0x02D1>(scriptHandle);
}

void CNetworkFireManager::HandleCreate(const CPackets::FireCreate& packet)
{
    if (packet.fireId == 0)
    {
        return;
    }

    auto it = ms_fires.find(packet.fireId);
    if (it != ms_fires.end())
    {
        it->second.state = packet;
        return;
    }

    FireEntity fire{};
    fire.scriptHandle = StartScriptFire(packet);
    if (fire.scriptHandle < 0)
    {
        return;
    }

    fire.state = packet;
    ms_fires.insert({ packet.fireId, fire });
}

void CNetworkFireManager::HandleUpdate(const CPackets::FireUpdate& packet)
{
    auto it = ms_fires.find(packet.fireId);
    if (it == ms_fires.end())
    {
        CPackets::FireCreate createPacket{};
        createPacket.fireId = packet.fireId;
        createPacket.position = packet.position;
        createPacket.radius = packet.radius;
        createPacket.fireType = packet.fireType;
        createPacket.ownerPlayerId = packet.ownerPlayerId;
        createPacket.sourceType = packet.sourceType;
        createPacket.timestampMs = packet.timestampMs;
        HandleCreate(createPacket);
        return;
    }

    const auto& previous = it->second.state;
    const float dx = previous.position.x - packet.position.x;
    const float dy = previous.position.y - packet.position.y;
    const float dz = previous.position.z - packet.position.z;
    const bool moved = dx * dx + dy * dy + dz * dz > 1.0f;
    const bool changedSize = std::abs(previous.radius - packet.radius) > 0.5f;

    if (moved || changedSize)
    {
        RemoveScriptFireSafe(it->second.scriptHandle);

        CPackets::FireCreate createPacket{};
        createPacket.fireId = packet.fireId;
        createPacket.position = packet.position;
        createPacket.radius = packet.radius;
        createPacket.fireType = packet.fireType;
        createPacket.ownerPlayerId = packet.ownerPlayerId;
        createPacket.sourceType = packet.sourceType;
        createPacket.timestampMs = packet.timestampMs;

        it->second.scriptHandle = StartScriptFire(createPacket);
        if (it->second.scriptHandle < 0)
        {
            ms_fires.erase(it);
            return;
        }
    }

    it->second.state.position = packet.position;
    it->second.state.radius = packet.radius;
    it->second.state.fireType = packet.fireType;
    it->second.state.ownerPlayerId = packet.ownerPlayerId;
    it->second.state.sourceType = packet.sourceType;
    it->second.state.timestampMs = packet.timestampMs;
}

void CNetworkFireManager::HandleRemove(const CPackets::FireRemove& packet)
{
    auto it = ms_fires.find(packet.fireId);
    if (it == ms_fires.end())
    {
        return;
    }

    RemoveScriptFireSafe(it->second.scriptHandle);
    ms_fires.erase(it);
}

void CNetworkFireManager::Reset()
{
    for (const auto& [id, fire] : ms_fires)
    {
        RemoveScriptFireSafe(fire.scriptHandle);
    }

    ms_fires.clear();
}
