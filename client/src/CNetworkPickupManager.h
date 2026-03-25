#pragma once

#include "stdafx.h"
#include <unordered_map>

class CNetworkPickupManager
{
public:
	struct Pickup
	{
		int id = -1;
		uint8_t type = 0;
		uint8_t category = 0;
		CVector position{};
		int modelId = 0;
		bool collected = false;
		uint32_t respawnMs = 0;
		int amount = 0;
		uint8_t weaponId = 0;
		uint16_t ammo = 0;
		uint32_t lastCollectAttemptTick = 0;
	};

	static void HandleCreate(const CPackets::PickupCreate& packet);
	static void HandleState(const CPackets::PickupStateChange& packet);
	static void Process();

private:
	static inline std::unordered_map<int, Pickup> ms_pickups{};
};
