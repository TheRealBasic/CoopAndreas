#include "CFireSyncManager.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <chrono>

#include "CNetwork.h"
#include "CPlayer.h"
#include "CPlayerManager.h"

std::unordered_map<uint32_t, CFireSyncManager::FireInstance> CFireSyncManager::ms_fires;
uint32_t CFireSyncManager::ms_nextFireId = 1;

namespace
{
    uint32_t NowMs()
    {
        return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    constexpr uint32_t FIRE_TTL_MS = 25000;
    constexpr float FIRE_STREAM_DISTANCE = 220.0f;
    constexpr int MAX_FIRES_PER_PLAYER = 12;
    constexpr int MAX_FIRES_IN_AREA = 24;
    constexpr float FIRE_AREA_RADIUS = 80.0f;
    constexpr float FIRE_DEDUP_RADIUS = 4.0f;
}

void CFireSyncManager::Process(uint32_t nowMs)
{
    std::vector<uint32_t> expiredIds;

    for (const auto& [id, fire] : ms_fires)
    {
        if (nowMs - fire.lastRefreshAtMs > FIRE_TTL_MS)
        {
            expiredIds.push_back(id);
        }
    }

    for (uint32_t id : expiredIds)
    {
        auto it = ms_fires.find(id);
        if (it == ms_fires.end())
        {
            continue;
        }

        BroadcastRemove(it->second);
        ms_fires.erase(it);
    }
}

void CFireSyncManager::OnExplosion(CPlayer* owner, const CVector& pos, uint8_t explosionType)
{
    const uint32_t nowMs = NowMs();
    const int ownerPlayerId = owner ? owner->m_iPlayerId : -1;
    const float radius = (explosionType == 0) ? 2.8f : 3.6f;
    CreateOrRefresh(ownerPlayerId, static_cast<uint8_t>(FireSourceType::Explosion), pos, radius, explosionType, nowMs);
}

void CFireSyncManager::OnProjectile(CPlayer* owner, const CVector& pos, uint8_t projectileType)
{
    const uint32_t nowMs = NowMs();
    const int ownerPlayerId = owner ? owner->m_iPlayerId : -1;
    CreateOrRefresh(ownerPlayerId, static_cast<uint8_t>(FireSourceType::Projectile), pos, 2.4f, projectileType, nowMs);
}

void CFireSyncManager::OnPlayerDisconnected(int playerId)
{
    std::vector<uint32_t> ownedFireIds;

    for (const auto& [id, fire] : ms_fires)
    {
        if (fire.ownerPlayerId == playerId)
        {
            ownedFireIds.push_back(id);
        }
    }

    for (uint32_t id : ownedFireIds)
    {
        auto it = ms_fires.find(id);
        if (it == ms_fires.end())
        {
            continue;
        }

        BroadcastRemove(it->second);
        ms_fires.erase(it);
    }
}

void CFireSyncManager::SendSnapshotTo(ENetPeer* peer, const CVector& playerPos)
{
    for (const auto& [id, fire] : ms_fires)
    {
        if (!IsInStreamRange(playerPos, fire.position))
        {
            continue;
        }

        CPlayerPackets::FireCreate packet{};
        packet.fireId = fire.id;
        packet.position = fire.position;
        packet.radius = fire.radius;
        packet.fireType = fire.fireType;
        packet.ownerPlayerId = fire.ownerPlayerId;
        packet.sourceType = fire.sourceType;
        packet.timestampMs = fire.lastRefreshAtMs;
        CNetwork::SendPacket(peer, CPacketsID::FIRE_CREATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }
}

CFireSyncManager::FireInstance* CFireSyncManager::FindNearby(const CVector& pos)
{
    for (auto& [id, fire] : ms_fires)
    {
        const float dx = fire.position.x - pos.x;
        const float dy = fire.position.y - pos.y;
        const float dz = fire.position.z - pos.z;
        if (dx * dx + dy * dy + dz * dz <= FIRE_DEDUP_RADIUS * FIRE_DEDUP_RADIUS)
        {
            return &fire;
        }
    }

    return nullptr;
}

bool CFireSyncManager::CanCreateForPlayer(int ownerPlayerId)
{
    if (ownerPlayerId < 0)
    {
        return true;
    }

    int ownedCount = 0;
    for (const auto& [id, fire] : ms_fires)
    {
        if (fire.ownerPlayerId == ownerPlayerId)
        {
            ownedCount++;
        }
    }

    return ownedCount < MAX_FIRES_PER_PLAYER;
}

bool CFireSyncManager::CanCreateInArea(const CVector& pos)
{
    int inAreaCount = 0;
    for (const auto& [id, fire] : ms_fires)
    {
        const float dx = fire.position.x - pos.x;
        const float dy = fire.position.y - pos.y;
        if (dx * dx + dy * dy <= FIRE_AREA_RADIUS * FIRE_AREA_RADIUS)
        {
            inAreaCount++;
        }
    }

    return inAreaCount < MAX_FIRES_IN_AREA;
}

void CFireSyncManager::CreateOrRefresh(int ownerPlayerId, uint8_t sourceType, const CVector& pos, float radius, uint8_t fireType, uint32_t nowMs)
{
    if (auto* existing = FindNearby(pos))
    {
        existing->position = pos;
        existing->radius = radius;
        existing->fireType = fireType;
        existing->lastRefreshAtMs = nowMs;
        BroadcastUpdate(*existing);
        return;
    }

    if (!CanCreateForPlayer(ownerPlayerId) || !CanCreateInArea(pos))
    {
        return;
    }

    FireInstance instance{};
    instance.id = ms_nextFireId++;
    instance.position = pos;
    instance.radius = std::clamp(radius, 0.8f, 12.0f);
    instance.fireType = fireType;
    instance.ownerPlayerId = ownerPlayerId;
    instance.sourceType = sourceType;
    instance.createdAtMs = nowMs;
    instance.lastRefreshAtMs = nowMs;

    ms_fires.insert({ instance.id, instance });
    BroadcastCreate(instance);
}

void CFireSyncManager::BroadcastCreate(const FireInstance& instance)
{
    CPlayerPackets::FireCreate packet{};
    packet.fireId = instance.id;
    packet.position = instance.position;
    packet.radius = instance.radius;
    packet.fireType = instance.fireType;
    packet.ownerPlayerId = instance.ownerPlayerId;
    packet.sourceType = instance.sourceType;
    packet.timestampMs = instance.lastRefreshAtMs;

    for (auto* player : CPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPeer)
        {
            continue;
        }

        if (!IsInStreamRange(player->m_vecPosition, instance.position))
        {
            continue;
        }

        CNetwork::SendPacket(player->m_pPeer, CPacketsID::FIRE_CREATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }
}

void CFireSyncManager::BroadcastUpdate(const FireInstance& instance)
{
    CPlayerPackets::FireUpdate packet{};
    packet.fireId = instance.id;
    packet.position = instance.position;
    packet.radius = instance.radius;
    packet.fireType = instance.fireType;
    packet.ownerPlayerId = instance.ownerPlayerId;
    packet.sourceType = instance.sourceType;
    packet.timestampMs = instance.lastRefreshAtMs;

    for (auto* player : CPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPeer)
        {
            continue;
        }

        if (!IsInStreamRange(player->m_vecPosition, instance.position))
        {
            continue;
        }

        CNetwork::SendPacket(player->m_pPeer, CPacketsID::FIRE_UPDATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }
}

void CFireSyncManager::BroadcastRemove(const FireInstance& instance)
{
    CPlayerPackets::FireRemove packet{};
    packet.fireId = instance.id;
    packet.timestampMs = NowMs();

    for (auto* player : CPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPeer)
        {
            continue;
        }

        CNetwork::SendPacket(player->m_pPeer, CPacketsID::FIRE_REMOVE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    }
}

bool CFireSyncManager::IsInStreamRange(const CVector& observerPos, const CVector& firePos)
{
    const float dx = observerPos.x - firePos.x;
    const float dy = observerPos.y - firePos.y;
    const float dz = observerPos.z - firePos.z;
    return dx * dx + dy * dy + dz * dz <= FIRE_STREAM_DISTANCE * FIRE_STREAM_DISTANCE;
}
