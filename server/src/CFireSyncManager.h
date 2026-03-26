#pragma once

#include <cstdint>
#include <unordered_map>
#include "CVector.h"
#include "enet/enet.h"

class CPlayer;

class CFireSyncManager
{
public:
    enum class FireSourceType : uint8_t
    {
        Explosion = 0,
        Projectile = 1,
    };

    struct FireInstance
    {
        uint32_t id{};
        CVector position{};
        float radius = 2.5f;
        uint8_t fireType = 0;
        int ownerPlayerId = -1;
        uint8_t sourceType = 0;
        uint32_t createdAtMs = 0;
        uint32_t lastRefreshAtMs = 0;
        uint32_t expiresAtMs = 0;
        uint64_t dedupKey = 0;
    };

    static void Process(uint32_t nowMs);
    static void OnExplosion(CPlayer* owner, const CVector& pos, uint8_t explosionType);
    static void OnProjectile(CPlayer* owner, const CVector& pos, uint8_t projectileType);
    static void OnPlayerDisconnected(int playerId);
    static void SendSnapshotTo(ENetPeer* peer, const CVector& playerPos);

private:
    static FireInstance* FindNearby(const CVector& pos);
    static FireInstance* FindDeterministicMatch(int ownerPlayerId, uint8_t sourceType, const CVector& pos, uint32_t nowMs);
    static bool CanCreateForPlayer(int ownerPlayerId);
    static bool CanCreateInArea(const CVector& pos);
    static uint64_t BuildDedupKey(int ownerPlayerId, uint8_t sourceType, const CVector& pos, uint32_t nowMs);
    static void CreateOrRefresh(int ownerPlayerId, uint8_t sourceType, const CVector& pos, float radius, uint8_t fireType, uint32_t nowMs);
    static void BroadcastCreate(const FireInstance& instance);
    static void BroadcastUpdate(const FireInstance& instance);
    static void BroadcastRemove(const FireInstance& instance);
    static void SetStreamState(int playerId, uint32_t fireId, bool isVisible);
    static bool IsStreamedToPlayer(int playerId, uint32_t fireId);
    static void ClearStreamStateForFire(uint32_t fireId);
    static bool IsInStreamRange(const CVector& observerPos, const CVector& firePos);

private:
    static std::unordered_map<uint32_t, FireInstance> ms_fires;
    static std::unordered_map<uint64_t, bool> ms_streamedState;
    static uint32_t ms_nextFireId;
};
