#include "stdafx.h"
#include "CNetworkPickupManager.h"

void CNetworkPickupManager::HandleCreate(const CPackets::PickupCreate& packet)
{
	Pickup pickup{};
	pickup.id = packet.pickupid;
	pickup.type = packet.type;
	pickup.category = packet.category;
	pickup.position = packet.position;
	pickup.modelId = packet.modelid;
	pickup.collected = packet.collected;
	pickup.respawnMs = packet.respawnMs;
	pickup.amount = packet.amount;
	pickup.weaponId = packet.weaponid;
	pickup.ammo = packet.ammo;
	ms_pickups[pickup.id] = pickup;
}

void CNetworkPickupManager::HandleState(const CPackets::PickupStateChange& packet)
{
	auto it = ms_pickups.find(packet.pickupid);
	if (it == ms_pickups.end())
	{
		return;
	}

	it->second.collected = packet.collected;
	if (!packet.collected)
	{
		it->second.lastCollectAttemptTick = 0;
	}
}

void CNetworkPickupManager::Process()
{
	if (!CNetwork::m_bConnected)
	{
		return;
	}

	auto* player = FindPlayerPed(0);
	if (!player)
	{
		return;
	}

	const uint32_t now = GetTickCount();

	for (auto& [id, pickup] : ms_pickups)
	{
		if (pickup.collected)
		{
			continue;
		}

		if (pickup.lastCollectAttemptTick != 0 && now - pickup.lastCollectAttemptTick < 750)
		{
			continue;
		}

		const float dx = player->GetPosition().x - pickup.position.x;
		const float dy = player->GetPosition().y - pickup.position.y;
		const float dz = player->GetPosition().z - pickup.position.z;
		const float sqDist = dx * dx + dy * dy + dz * dz;
		if (sqDist > 9.0f)
		{
			continue;
		}

		CPackets::PickupCollectRequest request{};
		request.pickupid = pickup.id;
		request.playerid = CNetworkPlayerManager::m_nMyId;
		request.playerPosition = player->GetPosition();
		CNetwork::SendPacket(CPacketsID::PICKUP_COLLECT_REQUEST, &request, sizeof(request), ENET_PACKET_FLAG_RELIABLE);
		pickup.lastCollectAttemptTick = now;
	}
}

void CNetworkPickupManager::Reset()
{
	ms_pickups.clear();
}
