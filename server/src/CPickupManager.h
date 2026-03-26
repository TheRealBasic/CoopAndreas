#pragma once

#ifndef _CPICKUPMANAGER_H_
#define _CPICKUPMANAGER_H_

#include <unordered_map>
#include <cstdint>

#include "CVector.h"
#include "CNetwork.h"
#include "CPacket.h"

class CPlayer;

class CPickupManager
{
public:
	enum ePickupType : uint8_t
	{
		PICKUP_TYPE_MONEY = 0,
		PICKUP_TYPE_WEAPON = 1,
		PICKUP_TYPE_JETPACK = 2,
		PICKUP_TYPE_CUSTOM = 3
	};

	enum ePickupFlags : uint8_t
	{
		PICKUP_FLAG_PERSISTENT = 1 << 0,
		PICKUP_FLAG_RESPAWNABLE = 1 << 1,
		PICKUP_FLAG_DROPPED = 1 << 2
	};

	struct Pickup
	{
		uint32_t networkId = 0;
		uint8_t type = PICKUP_TYPE_CUSTOM;
		uint8_t category = 0;
		uint32_t worldCollectibleId = 0;
		uint8_t origin = CPickupStatePackets::PICKUP_ORIGIN_COLLECTIBLE;
		uint8_t flags = 0;
		CVector position{};
		int modelId = 0;
		bool isSpawned = true;
		bool isCollected = false;
		int collectorPlayerId = -1;
		uint64_t stateTimestampMs = 0;
		uint32_t stateVersion = 0;
		uint32_t respawnMs = 0;
		uint64_t collectedAtMs = 0;
		int amount = 0;
		uint8_t weaponId = 0;
		uint16_t weaponAmmo = 0;
	};

	static uint32_t CreatePickup(uint8_t type, uint8_t category, const CVector& position, int modelId, uint32_t respawnMs = 0, int amount = 0, uint8_t weaponId = 0, uint16_t weaponAmmo = 0, uint8_t origin = CPickupStatePackets::PICKUP_ORIGIN_COLLECTIBLE, uint8_t flags = PICKUP_FLAG_PERSISTENT | PICKUP_FLAG_RESPAWNABLE, uint32_t worldCollectibleId = 0);
	static void SendSnapshot(ENetPeer* peer);
	static void HandleCollectRequest(ENetPeer* peer, uint32_t pickupId, const CVector& reportedPosition, uint32_t knownStateVersion);
	static void ProcessRespawns();
	static void CreateDropsForPlayerDeath(CPlayer* player);

private:
	static inline std::unordered_map<uint32_t, Pickup> ms_pickups{};
	static inline std::unordered_map<uint64_t, Pickup> ms_collectibleStateByKey{};
	static inline uint32_t ms_nextPickupId = 1;
	static inline uint32_t ms_snapshotVersion = 1;

	static uint64_t BuildCollectibleStateKey(uint8_t category, uint32_t worldCollectibleId);
	static bool IsWorldCollectible(const Pickup& pickup);
	static void PersistCollectibleState(const Pickup& pickup);
	static uint64_t GetServerTimeMs();
	static void UpdateStateMetadata(Pickup& pickup);
	static CPickupStatePackets::PickupSnapshotEntry BuildSnapshotEntry(const Pickup& pickup);
	static void BroadcastDelta(const Pickup& pickup, uint8_t action);
	static void BroadcastDropCreate(const Pickup& pickup);
	static void BroadcastDropResolve(const Pickup& pickup, uint8_t action, int resolverPlayerId);
};

class CPickupPackets
{
public:
	struct PickupCollectRequest : public CPickupStatePackets::PickupCollectRequest
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* packet = (CPickupStatePackets::PickupCollectRequest*)data;
			CPickupManager::HandleCollectRequest(peer, packet->networkId, packet->playerPosition, packet->knownStateVersion);
		}
	};
};

#endif
