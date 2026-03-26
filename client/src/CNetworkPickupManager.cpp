#include "stdafx.h"
#include "CNetworkPickupManager.h"

namespace
{
	enum ePickupAction : uint8_t
	{
		PICKUP_ACTION_SPAWN = 0,
		PICKUP_ACTION_COLLECT = 1,
		PICKUP_ACTION_REMOVE = 2
	};
}

void CNetworkPickupManager::UpsertFromSnapshotEntry(const CPackets::PickupSnapshotEntry& packet)
{
	Pickup pickup{};
	pickup.networkId = packet.networkId;
	pickup.type = packet.type;
	pickup.category = packet.category;
	pickup.origin = packet.origin;
	pickup.flags = packet.flags;
	pickup.position = packet.position;
	pickup.modelId = packet.modelId;
	pickup.isSpawned = packet.isSpawned;
	pickup.isCollected = packet.isCollected;
	pickup.collectorPlayerId = packet.collectorPlayerId;
	pickup.stateTimestampMs = packet.stateTimestampMs;
	pickup.stateVersion = packet.stateVersion;
	pickup.respawnMs = packet.respawnMs;
	pickup.amount = packet.amount;
	pickup.weaponId = packet.weaponId;
	pickup.ammo = packet.weaponAmmo;
	ms_pickups[pickup.networkId] = pickup;
}

void CNetworkPickupManager::HandleSnapshotBegin(const CPackets::PickupSnapshotBegin& packet)
{
	ms_snapshotInProgress = true;
	ms_snapshotReadyForInteraction = false;
	ms_pickups.clear();
}

void CNetworkPickupManager::HandleSnapshotEntry(const CPackets::PickupSnapshotEntry& packet)
{
	UpsertFromSnapshotEntry(packet);
}

void CNetworkPickupManager::HandleSnapshotEnd(const CPackets::PickupSnapshotEnd& packet)
{
	ms_snapshotInProgress = false;
	ms_snapshotReadyForInteraction = true;
}

void CNetworkPickupManager::HandleStateDelta(const CPackets::PickupStateDelta& packet)
{
	auto it = ms_pickups.find(packet.networkId);
	if (it == ms_pickups.end())
	{
		if (packet.action != PICKUP_ACTION_REMOVE)
		{
			Pickup pickup{};
			pickup.networkId = packet.networkId;
			pickup.stateVersion = packet.stateVersion;
			pickup.stateTimestampMs = packet.stateTimestampMs;
			pickup.isSpawned = packet.isSpawned;
			pickup.isCollected = packet.isCollected;
			pickup.collectorPlayerId = packet.collectorPlayerId;
			ms_pickups[pickup.networkId] = pickup;
		}
		return;
	}

	if (packet.stateVersion < it->second.stateVersion)
	{
		return;
	}

	if (packet.action == PICKUP_ACTION_REMOVE)
	{
		ms_pickups.erase(it);
		return;
	}

	it->second.isSpawned = packet.isSpawned;
	it->second.isCollected = packet.isCollected;
	it->second.collectorPlayerId = packet.collectorPlayerId;
	it->second.stateTimestampMs = packet.stateTimestampMs;
	it->second.stateVersion = packet.stateVersion;
	if (!packet.isCollected)
	{
		it->second.lastCollectAttemptTick = 0;
	}
}

void CNetworkPickupManager::HandleDropCreate(const CPackets::PickupDropCreate& packet)
{
	UpsertFromSnapshotEntry(packet.pickup);
}

void CNetworkPickupManager::HandleDropResolve(const CPackets::PickupDropResolve& packet)
{
	auto it = ms_pickups.find(packet.networkId);
	if (it == ms_pickups.end())
	{
		return;
	}

	if (packet.stateVersion < it->second.stateVersion)
	{
		return;
	}

	if (packet.action == PICKUP_ACTION_REMOVE || packet.action == PICKUP_ACTION_COLLECT)
	{
		ms_pickups.erase(it);
		return;
	}
}

void CNetworkPickupManager::Process()
{
	if (!CNetwork::m_bConnected || ms_snapshotInProgress || !ms_snapshotReadyForInteraction)
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
		if (!pickup.isSpawned || pickup.isCollected)
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
		request.networkId = pickup.networkId;
		request.playerId = CNetworkPlayerManager::m_nMyId;
		request.playerPosition = player->GetPosition();
		request.knownStateVersion = pickup.stateVersion;
		CNetwork::SendPacket(CPacketsID::PICKUP_COLLECT_REQUEST, &request, sizeof(request), ENET_PACKET_FLAG_RELIABLE);
		pickup.lastCollectAttemptTick = now;
	}
}

void CNetworkPickupManager::Reset()
{
	ms_pickups.clear();
	ms_snapshotInProgress = false;
	ms_snapshotReadyForInteraction = false;
}

void CNetworkPickupManager::BeginResync()
{
	ms_snapshotInProgress = true;
	ms_snapshotReadyForInteraction = false;
	ms_pickups.clear();
}

bool CNetworkPickupManager::IsReadyForInteraction()
{
	return ms_snapshotReadyForInteraction && !ms_snapshotInProgress;
}
