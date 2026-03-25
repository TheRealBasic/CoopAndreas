#include "CPickupManager.h"

#include <algorithm>
#include <chrono>

#include "CPlayer.h"
#include "CPlayerManager.h"

uint64_t CPickupManager::GetServerTimeMs()
{
	const auto now = std::chrono::steady_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	return (uint64_t)ms.count();
}

int CPickupManager::CreatePickup(uint8_t type, uint8_t category, const CVector& position, int modelId, uint32_t respawnMs, int amount, uint8_t weaponId, uint16_t weaponAmmo)
{
	Pickup pickup{};
	pickup.id = ms_nextPickupId++;
	pickup.type = type;
	pickup.category = category;
	pickup.position = position;
	pickup.modelId = modelId;
	pickup.respawnMs = respawnMs;
	pickup.amount = std::max(0, amount);
	pickup.weaponId = weaponId;
	pickup.weaponAmmo = weaponAmmo;

	ms_pickups[pickup.id] = pickup;
	BroadcastCreate(pickup);
	return pickup.id;
}

void CPickupManager::BroadcastCreate(const Pickup& pickup)
{
	CPickupPackets::PickupCreate packet{};
	packet.pickupid = pickup.id;
	packet.type = pickup.type;
	packet.category = pickup.category;
	packet.position = pickup.position;
	packet.modelid = pickup.modelId;
	packet.collected = pickup.collected;
	packet.respawnMs = pickup.respawnMs;
	packet.amount = pickup.amount;
	packet.weaponid = pickup.weaponId;
	packet.ammo = pickup.weaponAmmo;
	CNetwork::SendPacketToAll(CPacketsID::PICKUP_CREATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
}

void CPickupManager::BroadcastState(const Pickup& pickup, int collectorPlayerId)
{
	CPickupPackets::PickupStateChange packet{};
	packet.pickupid = pickup.id;
	packet.collected = pickup.collected;
	packet.collectorPlayerId = collectorPlayerId;
	CNetwork::SendPacketToAll(CPacketsID::PICKUP_STATE_CHANGE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
}

void CPickupManager::SendSnapshot(ENetPeer* peer)
{
	for (const auto& [id, pickup] : ms_pickups)
	{
		CPickupPackets::PickupCreate createPacket{};
		createPacket.pickupid = id;
		createPacket.type = pickup.type;
		createPacket.category = pickup.category;
		createPacket.position = pickup.position;
		createPacket.modelid = pickup.modelId;
		createPacket.collected = pickup.collected;
		createPacket.respawnMs = pickup.respawnMs;
		createPacket.amount = pickup.amount;
		createPacket.weaponid = pickup.weaponId;
		createPacket.ammo = pickup.weaponAmmo;
		CNetwork::SendPacket(peer, CPacketsID::PICKUP_CREATE, &createPacket, sizeof(createPacket), ENET_PACKET_FLAG_RELIABLE);
	}
}

void CPickupManager::HandleCollectRequest(ENetPeer* peer, int pickupId, const CVector& reportedPosition)
{
	auto* player = CPlayerManager::GetPlayer(peer);
	if (!player)
	{
		return;
	}

	auto it = ms_pickups.find(pickupId);
	if (it == ms_pickups.end())
	{
		return;
	}

	auto& pickup = it->second;
	if (pickup.collected)
	{
		return;
	}

	const CVector authoritativePos = player->m_vecPosition;
	const float dx = authoritativePos.x - pickup.position.x;
	const float dy = authoritativePos.y - pickup.position.y;
	const float dz = authoritativePos.z - pickup.position.z;
	const float sqDist = (dx * dx) + (dy * dy) + (dz * dz);

	const float reportedDx = reportedPosition.x - pickup.position.x;
	const float reportedDy = reportedPosition.y - pickup.position.y;
	const float reportedDz = reportedPosition.z - pickup.position.z;
	const float reportedSqDist = (reportedDx * reportedDx) + (reportedDy * reportedDy) + (reportedDz * reportedDz);

	if (sqDist > 25.0f || reportedSqDist > 64.0f)
	{
		return;
	}

	pickup.collected = true;
	pickup.collectedAtMs = GetServerTimeMs();
	BroadcastState(pickup, player->m_iPlayerId);
}

void CPickupManager::ProcessRespawns()
{
	const uint64_t nowMs = GetServerTimeMs();
	for (auto& [id, pickup] : ms_pickups)
	{
		if (!pickup.collected || pickup.respawnMs == 0)
		{
			continue;
		}

		if (nowMs - pickup.collectedAtMs < pickup.respawnMs)
		{
			continue;
		}

		pickup.collected = false;
		pickup.collectedAtMs = 0;
		BroadcastState(pickup, -1);
	}
}

void CPickupManager::CreateDropsForPlayerDeath(CPlayer* player)
{
	if (!player)
	{
		return;
	}

	const CVector dropPos = player->m_vecPosition;

	if (player->m_nMoney > 0)
	{
		int dropMoney = std::clamp(player->m_nMoney / 4, 50, 5000);
		CreatePickup(PICKUP_TYPE_MONEY, 0, dropPos, 1212, 60000, dropMoney);
	}

	const bool isValidWeapon = (player->m_ucCurrentWeapon >= 22 && player->m_ucCurrentWeapon <= 46);
	if (isValidWeapon && player->m_usCurrentAmmo > 0)
	{
		CreatePickup(PICKUP_TYPE_WEAPON, 0, dropPos, player->m_ucCurrentWeapon, 45000, 0, player->m_ucCurrentWeapon, player->m_usCurrentAmmo);
	}

	if (player->m_bHasJetpack)
	{
		CreatePickup(PICKUP_TYPE_JETPACK, 0, dropPos, 370, 90000);
	}
}
