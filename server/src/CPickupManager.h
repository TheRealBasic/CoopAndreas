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

	struct Pickup
	{
		int id = -1;
		uint8_t type = PICKUP_TYPE_CUSTOM;
		uint8_t category = 0;
		CVector position{};
		int modelId = 0;
		bool collected = false;
		uint32_t respawnMs = 0;
		uint64_t collectedAtMs = 0;
		int amount = 0;
		uint8_t weaponId = 0;
		uint16_t weaponAmmo = 0;
	};

	static int CreatePickup(uint8_t type, uint8_t category, const CVector& position, int modelId, uint32_t respawnMs = 0, int amount = 0, uint8_t weaponId = 0, uint16_t weaponAmmo = 0);
	static void SendSnapshot(ENetPeer* peer);
	static void HandleCollectRequest(ENetPeer* peer, int pickupId, const CVector& reportedPosition);
	static void ProcessRespawns();
	static void CreateDropsForPlayerDeath(CPlayer* player);

private:
	static inline std::unordered_map<int, Pickup> ms_pickups{};
	static inline int ms_nextPickupId = 1;

	static uint64_t GetServerTimeMs();
	static void BroadcastCreate(const Pickup& pickup);
	static void BroadcastState(const Pickup& pickup, int collectorPlayerId);
};

class CPickupPackets
{
public:
#pragma pack(1)
	struct PickupCreate
	{
		int pickupid;
		uint8_t type;
		uint8_t category;
		CVector position;
		int modelid;
		bool collected;
		uint32_t respawnMs;
		int amount;
		uint8_t weaponid;
		uint16_t ammo;
	};

	struct PickupCollectRequest
	{
		int pickupid;
		int playerid;
		CVector playerPosition;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* packet = (PickupCollectRequest*)data;
			CPickupManager::HandleCollectRequest(peer, packet->pickupid, packet->playerPosition);
		}
	};

	struct PickupStateChange
	{
		int pickupid;
		bool collected;
		int collectorPlayerId;
	};
#pragma pack()
};

#endif
