#pragma once

#include "stdafx.h"
#include <unordered_map>

class CNetworkPickupManager
{
public:
	struct Pickup
	{
		uint32_t networkId = 0;
		uint8_t type = 0;
		uint8_t category = 0;
		uint8_t origin = 0;
		uint8_t flags = 0;
		CVector position{};
		int modelId = 0;
		bool isSpawned = true;
		bool isCollected = false;
		int collectorPlayerId = -1;
		uint64_t stateTimestampMs = 0;
		uint32_t stateVersion = 0;
		uint32_t respawnMs = 0;
		int amount = 0;
		uint8_t weaponId = 0;
		uint16_t ammo = 0;
		uint32_t lastCollectAttemptTick = 0;
	};

	static void HandleSnapshotBegin(const CPackets::PickupSnapshotBegin& packet);
	static void HandleSnapshotEntry(const CPackets::PickupSnapshotEntry& packet);
	static void HandleSnapshotEnd(const CPackets::PickupSnapshotEnd& packet);
	static void HandleStateDelta(const CPackets::PickupStateDelta& packet);
	static void HandleDropCreate(const CPackets::PickupDropCreate& packet);
	static void HandleDropResolve(const CPackets::PickupDropResolve& packet);
	static void BeginResync();
	static bool IsReadyForInteraction();
	static void Process();
	static void Reset();

private:
	static void UpsertFromSnapshotEntry(const CPackets::PickupSnapshotEntry& packet);
	static inline std::unordered_map<uint32_t, Pickup> ms_pickups{};
	static inline bool ms_snapshotInProgress = false;
	static inline bool ms_snapshotReadyForInteraction = false;
};
