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
    };

    static void Process(uint32_t nowMs);
    static void OnExplosion(CPlayer* owner, const CVector& pos, uint8_t explosionType);
    static void OnProjectile(CPlayer* owner, const CVector& pos, uint8_t projectileType);
    static void OnPlayerDisconnected(int playerId);
    static void SendSnapshotTo(ENetPeer* peer, const CVector& playerPos);

private:
    static FireInstance* FindNearby(const CVector& pos);
    static bool CanCreateForPlayer(int ownerPlayerId);
    static bool CanCreateInArea(const CVector& pos);
    static void CreateOrRefresh(int ownerPlayerId, uint8_t sourceType, const CVector& pos, float radius, uint8_t fireType, uint32_t nowMs);
    static void BroadcastCreate(const FireInstance& instance);
    static void BroadcastUpdate(const FireInstance& instance);
    static void BroadcastRemove(const FireInstance& instance);
    static bool IsInStreamRange(const CVector& observerPos, const CVector& firePos);

private:
    static std::unordered_map<uint32_t, FireInstance> ms_fires;
    static uint32_t ms_nextFireId;
};
