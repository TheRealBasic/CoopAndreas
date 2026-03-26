#include "CFireSyncManager.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <chrono>
#include <cstdio>

#include "CNetwork.h"
#include "CPlayer.h"
#include "CPlayerManager.h"

std::unordered_map<uint32_t, CFireSyncManager::FireInstance> CFireSyncManager::ms_fires;
std::unordered_map<uint64_t, bool> CFireSyncManager::ms_streamedState;
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
    constexpr uint32_t FIRE_SOURCE_WINDOW_MS = 350;
    constexpr float FIRE_DEDUP_POSITION_TOLERANCE = 2.5f;
    constexpr float FIRE_STREAM_ENTER_DISTANCE = FIRE_STREAM_DISTANCE;
    constexpr float FIRE_STREAM_EXIT_DISTANCE = FIRE_STREAM_DISTANCE + 20.0f;

    uint64_t MakeStreamStateKey(int playerId, uint32_t fireId)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(playerId)) << 32) | fireId;
    }

    bool IsExpired(const CFireSyncManager::FireInstance& fire, uint32_t nowMs)
    {
        return nowMs >= fire.expiresAtMs;
    }

    float DistSq3D(const CVector& a, const CVector& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }
}

void CFireSyncManager::Process(uint32_t nowMs)
{
    std::vector<uint32_t> expiredIds;

    for (const auto& [id, fire] : ms_fires)
    {
        if (IsExpired(fire, nowMs))
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

        std::printf("[FireSync] Removing expired fireId=%u owner=%d src=%u createdAt=%u expiresAt=%u now=%u\n",
            it->second.id, it->second.ownerPlayerId, it->second.sourceType, it->second.createdAtMs, it->second.expiresAtMs, nowMs);
        BroadcastRemove(it->second);
        ClearStreamStateForFire(id);
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
        ClearStreamStateForFire(id);
        ms_fires.erase(it);
    }
}

void CFireSyncManager::SendSnapshotTo(ENetPeer* peer, const CVector& playerPos)
{
    const uint32_t nowMs = NowMs();
    std::vector<uint32_t> expiredIds;
    auto* snapshotPlayer = CPlayerManager::GetPlayer(peer);
    const int snapshotPlayerId = snapshotPlayer ? snapshotPlayer->m_iPlayerId : -1;

    for (const auto& [id, fire] : ms_fires)
    {
        if (IsExpired(fire, nowMs))
        {
            expiredIds.push_back(id);
            continue;
        }

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
        packet.timestampMs = fire.expiresAtMs;
        CNetwork::SendPacket(peer, CPacketsID::FIRE_CREATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);

        if (snapshotPlayerId >= 0)
        {
            SetStreamState(snapshotPlayerId, fire.id, true);
        }
    }

    for (uint32_t id : expiredIds)
    {
        auto it = ms_fires.find(id);
        if (it == ms_fires.end())
        {
            continue;
        }

        std::printf("[FireSync] Snapshot cleanup removed expired fireId=%u owner=%d src=%u createdAt=%u expiresAt=%u now=%u\n",
            it->second.id, it->second.ownerPlayerId, it->second.sourceType, it->second.createdAtMs, it->second.expiresAtMs, nowMs);
        BroadcastRemove(it->second);
        ClearStreamStateForFire(id);
        ms_fires.erase(it);
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

    if (ownedCount >= MAX_FIRES_PER_PLAYER)
    {
        std::printf("[FireSync] Reject create: CanCreateForPlayer=false owner=%d owned=%d limit=%d\n",
            ownerPlayerId, ownedCount, MAX_FIRES_PER_PLAYER);
        return false;
    }

    return true;
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

    if (inAreaCount >= MAX_FIRES_IN_AREA)
    {
        std::printf("[FireSync] Reject create: CanCreateInArea=false pos=(%.2f,%.2f,%.2f) nearby=%d radius=%.2f limit=%d\n",
            pos.x, pos.y, pos.z, inAreaCount, FIRE_AREA_RADIUS, MAX_FIRES_IN_AREA);
        return false;
    }

    return true;
}

void CFireSyncManager::CreateOrRefresh(int ownerPlayerId, uint8_t sourceType, const CVector& pos, float radius, uint8_t fireType, uint32_t nowMs)
{
    if (auto* existing = FindDeterministicMatch(ownerPlayerId, sourceType, pos, nowMs))
    {
        existing->position = pos;
        existing->radius = std::clamp(radius, 0.8f, 12.0f);
        existing->fireType = fireType;
        existing->ownerPlayerId = ownerPlayerId;
        existing->sourceType = sourceType;
        existing->lastRefreshAtMs = nowMs;
        existing->expiresAtMs = nowMs + FIRE_TTL_MS;
        existing->dedupKey = BuildDedupKey(ownerPlayerId, sourceType, pos, nowMs);
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
    instance.expiresAtMs = nowMs + FIRE_TTL_MS;
    instance.dedupKey = BuildDedupKey(ownerPlayerId, sourceType, pos, nowMs);

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
    packet.timestampMs = instance.expiresAtMs;

    for (auto* player : CPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPeer)
        {
            continue;
        }

        const bool isVisible = IsStreamedToPlayer(player->m_iPlayerId, instance.id);
        const float streamDistance = isVisible ? FIRE_STREAM_EXIT_DISTANCE : FIRE_STREAM_ENTER_DISTANCE;
        const float distSq = DistSq3D(player->m_vecPosition, instance.position);
        if (distSq > streamDistance * streamDistance)
        {
            SetStreamState(player->m_iPlayerId, instance.id, false);
            continue;
        }

        SetStreamState(player->m_iPlayerId, instance.id, true);
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
    packet.timestampMs = instance.expiresAtMs;

    for (auto* player : CPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPeer)
        {
            continue;
        }

        const bool isVisible = IsStreamedToPlayer(player->m_iPlayerId, instance.id);
        const float distSq = DistSq3D(player->m_vecPosition, instance.position);
        if (!isVisible)
        {
            if (distSq <= FIRE_STREAM_ENTER_DISTANCE * FIRE_STREAM_ENTER_DISTANCE)
            {
                CPlayerPackets::FireCreate createPacket{};
                createPacket.fireId = instance.id;
                createPacket.position = instance.position;
                createPacket.radius = instance.radius;
                createPacket.fireType = instance.fireType;
                createPacket.ownerPlayerId = instance.ownerPlayerId;
                createPacket.sourceType = instance.sourceType;
                createPacket.timestampMs = instance.expiresAtMs;
                CNetwork::SendPacket(player->m_pPeer, CPacketsID::FIRE_CREATE, &createPacket, sizeof(createPacket), ENET_PACKET_FLAG_RELIABLE);
                SetStreamState(player->m_iPlayerId, instance.id, true);
            }
            continue;
        }

        if (distSq > FIRE_STREAM_EXIT_DISTANCE * FIRE_STREAM_EXIT_DISTANCE)
        {
            CPlayerPackets::FireRemove removePacket{};
            removePacket.fireId = instance.id;
            removePacket.timestampMs = NowMs();
            CNetwork::SendPacket(player->m_pPeer, CPacketsID::FIRE_REMOVE, &removePacket, sizeof(removePacket), ENET_PACKET_FLAG_RELIABLE);
            SetStreamState(player->m_iPlayerId, instance.id, false);
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
        SetStreamState(player->m_iPlayerId, instance.id, false);
    }
}

CFireSyncManager::FireInstance* CFireSyncManager::FindDeterministicMatch(int ownerPlayerId, uint8_t sourceType, const CVector& pos, uint32_t nowMs)
{
    const uint64_t dedupKey = BuildDedupKey(ownerPlayerId, sourceType, pos, nowMs);
    const float toleranceSq = FIRE_DEDUP_POSITION_TOLERANCE * FIRE_DEDUP_POSITION_TOLERANCE;

    for (auto& [id, fire] : ms_fires)
    {
        if (nowMs > fire.lastRefreshAtMs + FIRE_SOURCE_WINDOW_MS)
        {
            continue;
        }

        if (fire.dedupKey != dedupKey)
        {
            continue;
        }

        if (DistSq3D(fire.position, pos) <= toleranceSq)
        {
            return &fire;
        }
    }

    return FindNearby(pos);
}

uint64_t CFireSyncManager::BuildDedupKey(int ownerPlayerId, uint8_t sourceType, const CVector& pos, uint32_t nowMs)
{
    const int32_t qx = static_cast<int32_t>(std::floor(pos.x / FIRE_DEDUP_POSITION_TOLERANCE));
    const int32_t qy = static_cast<int32_t>(std::floor(pos.y / FIRE_DEDUP_POSITION_TOLERANCE));
    const int32_t qz = static_cast<int32_t>(std::floor(pos.z / FIRE_DEDUP_POSITION_TOLERANCE));
    const uint32_t sourceWindow = nowMs / FIRE_SOURCE_WINDOW_MS;

    uint64_t key = 1469598103934665603ull;
    auto hashMix = [&key](uint32_t value)
    {
        key ^= value;
        key *= 1099511628211ull;
    };

    hashMix(static_cast<uint32_t>(ownerPlayerId + 2048));
    hashMix(static_cast<uint32_t>(sourceType));
    hashMix(static_cast<uint32_t>(qx));
    hashMix(static_cast<uint32_t>(qy));
    hashMix(static_cast<uint32_t>(qz));
    hashMix(sourceWindow);
    return key;
}

void CFireSyncManager::SetStreamState(int playerId, uint32_t fireId, bool isVisible)
{
    const uint64_t key = MakeStreamStateKey(playerId, fireId);
    if (isVisible)
    {
        ms_streamedState[key] = true;
        return;
    }

    ms_streamedState.erase(key);
}

bool CFireSyncManager::IsStreamedToPlayer(int playerId, uint32_t fireId)
{
    auto it = ms_streamedState.find(MakeStreamStateKey(playerId, fireId));
    return it != ms_streamedState.end() && it->second;
}

void CFireSyncManager::ClearStreamStateForFire(uint32_t fireId)
{
    for (auto it = ms_streamedState.begin(); it != ms_streamedState.end();)
    {
        if (static_cast<uint32_t>(it->first & 0xFFFFFFFFu) == fireId)
        {
            it = ms_streamedState.erase(it);
            continue;
        }

        ++it;
    }
}

bool CFireSyncManager::IsInStreamRange(const CVector& observerPos, const CVector& firePos)
{
    const float dx = observerPos.x - firePos.x;
    const float dy = observerPos.y - firePos.y;
    const float dz = observerPos.z - firePos.z;
    return dx * dx + dy * dy + dz * dz <= FIRE_STREAM_DISTANCE * FIRE_STREAM_DISTANCE;
}
