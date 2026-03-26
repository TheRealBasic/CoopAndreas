#include "CPickupManager.h"

#include <algorithm>
#include <chrono>

#include "CPlayer.h"
#include "CPlayerManager.h"

uint64_t CPickupManager::BuildCollectibleStateKey(uint8_t category, uint32_t worldCollectibleId)
{
	return (static_cast<uint64_t>(category) << 32) | static_cast<uint64_t>(worldCollectibleId);
}

bool CPickupManager::IsWorldCollectible(const Pickup& pickup)
{
	return pickup.origin == CPickupStatePackets::PICKUP_ORIGIN_COLLECTIBLE && pickup.worldCollectibleId != 0;
}

void CPickupManager::PersistCollectibleState(const Pickup& pickup)
{
	if (!IsWorldCollectible(pickup))
	{
		return;
	}

	ms_collectibleStateByKey[BuildCollectibleStateKey(pickup.category, pickup.worldCollectibleId)] = pickup;
}

uint64_t CPickupManager::GetServerTimeMs()
{
	const auto now = std::chrono::steady_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
	return (uint64_t)ms.count();
}

void CPickupManager::UpdateStateMetadata(Pickup& pickup)
{
	pickup.stateTimestampMs = GetServerTimeMs();
	pickup.stateVersion++;
}

CPickupStatePackets::PickupSnapshotEntry CPickupManager::BuildSnapshotEntry(const Pickup& pickup)
{
	CPickupStatePackets::PickupSnapshotEntry packet{};
	packet.networkId = pickup.networkId;
	packet.type = pickup.type;
	packet.category = pickup.category;
	packet.worldCollectibleId = pickup.worldCollectibleId;
	packet.origin = pickup.origin;
	packet.flags = pickup.flags;
	packet.position = pickup.position;
	packet.modelId = pickup.modelId;
	packet.amount = pickup.amount;
	packet.weaponId = pickup.weaponId;
	packet.weaponAmmo = pickup.weaponAmmo;
	packet.respawnMs = pickup.respawnMs;
	packet.isSpawned = pickup.isSpawned;
	packet.isCollected = pickup.isCollected;
	packet.collectorPlayerId = pickup.collectorPlayerId;
	packet.stateTimestampMs = pickup.stateTimestampMs;
	packet.stateVersion = pickup.stateVersion;
	return packet;
}

uint32_t CPickupManager::CreatePickup(uint8_t type, uint8_t category, const CVector& position, int modelId, uint32_t respawnMs, int amount, uint8_t weaponId, uint16_t weaponAmmo, uint8_t origin, uint8_t flags, uint32_t worldCollectibleId)
{
	Pickup pickup{};
	pickup.networkId = ms_nextPickupId++;
	pickup.type = type;
	pickup.category = category;
	pickup.worldCollectibleId = worldCollectibleId;
	pickup.origin = origin;
	pickup.flags = flags;
	pickup.position = position;
	pickup.modelId = modelId;
	pickup.respawnMs = respawnMs;
	pickup.amount = std::max(0, amount);
	pickup.weaponId = weaponId;
	pickup.weaponAmmo = weaponAmmo;
	pickup.isSpawned = true;
	pickup.isCollected = false;
	pickup.collectorPlayerId = -1;
	UpdateStateMetadata(pickup);

	if (IsWorldCollectible(pickup))
	{
		const auto persistedState = ms_collectibleStateByKey.find(BuildCollectibleStateKey(pickup.category, pickup.worldCollectibleId));
		if (persistedState != ms_collectibleStateByKey.end())
		{
			pickup.isSpawned = persistedState->second.isSpawned;
			pickup.isCollected = persistedState->second.isCollected;
			pickup.collectorPlayerId = persistedState->second.collectorPlayerId;
			pickup.collectedAtMs = persistedState->second.collectedAtMs;
			pickup.stateTimestampMs = persistedState->second.stateTimestampMs;
			pickup.stateVersion = persistedState->second.stateVersion;
		}
	}

	ms_pickups[pickup.networkId] = pickup;
	PersistCollectibleState(pickup);
	ms_snapshotVersion++;
	BroadcastDelta(pickup, pickup.isCollected ? CPickupStatePackets::PICKUP_ACTION_COLLECT : CPickupStatePackets::PICKUP_ACTION_SPAWN);

	if ((pickup.flags & PICKUP_FLAG_DROPPED) != 0)
	{
		BroadcastDropCreate(pickup);
	}

	return pickup.networkId;
}

void CPickupManager::BroadcastDelta(const Pickup& pickup, uint8_t action)
{
	CPickupStatePackets::PickupStateDelta packet{};
	packet.networkId = pickup.networkId;
	packet.action = action;
	packet.isSpawned = pickup.isSpawned;
	packet.isCollected = pickup.isCollected;
	packet.collectorPlayerId = pickup.collectorPlayerId;
	packet.stateTimestampMs = pickup.stateTimestampMs;
	packet.stateVersion = pickup.stateVersion;
	CNetwork::SendPacketToAll(CPacketsID::PICKUP_STATE_DELTA, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
}

void CPickupManager::BroadcastDropCreate(const Pickup& pickup)
{
	CPickupStatePackets::PickupDropCreate packet{};
	packet.pickup = BuildSnapshotEntry(pickup);
	CNetwork::SendPacketToAll(CPacketsID::PICKUP_DROP_CREATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
}

void CPickupManager::BroadcastDropResolve(const Pickup& pickup, uint8_t action, int resolverPlayerId)
{
	CPickupStatePackets::PickupDropResolve packet{};
	packet.networkId = pickup.networkId;
	packet.action = action;
	packet.resolverPlayerId = resolverPlayerId;
	packet.stateTimestampMs = pickup.stateTimestampMs;
	packet.stateVersion = pickup.stateVersion;
	CNetwork::SendPacketToAll(CPacketsID::PICKUP_DROP_RESOLVE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
}

void CPickupManager::RejectDropCollectClaim(Pickup& pickup, int resolverPlayerId)
{
	UpdateStateMetadata(pickup);
	BroadcastDropResolve(pickup, CPickupStatePackets::PICKUP_DROP_RESOLVE_ACTION_REJECTED, resolverPlayerId);
}

void CPickupManager::SendSnapshot(ENetPeer* peer)
{
	CPickupStatePackets::PickupSnapshotBegin beginPacket{};
	beginPacket.snapshotVersion = ms_snapshotVersion;
	beginPacket.pickupCount = (uint32_t)ms_pickups.size();
	CNetwork::SendPacket(peer, CPacketsID::PICKUP_SNAPSHOT_BEGIN, &beginPacket, sizeof(beginPacket), ENET_PACKET_FLAG_RELIABLE);

	for (const auto& [id, pickup] : ms_pickups)
	{
		auto entryPacket = BuildSnapshotEntry(pickup);
		CNetwork::SendPacket(peer, CPacketsID::PICKUP_SNAPSHOT_ENTRY, &entryPacket, sizeof(entryPacket), ENET_PACKET_FLAG_RELIABLE);
	}

	CPickupStatePackets::PickupSnapshotEnd endPacket{};
	endPacket.snapshotVersion = ms_snapshotVersion;
	CNetwork::SendPacket(peer, CPacketsID::PICKUP_SNAPSHOT_END, &endPacket, sizeof(endPacket), ENET_PACKET_FLAG_RELIABLE);
}

void CPickupManager::HandleCollectRequest(ENetPeer* peer, uint32_t pickupId, const CVector& reportedPosition, uint32_t knownStateVersion)
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
	const bool isDroppedPickup = (pickup.flags & PICKUP_FLAG_DROPPED) != 0;
	if (!pickup.isSpawned || pickup.isCollected)
	{
		if (isDroppedPickup)
		{
			RejectDropCollectClaim(pickup, player->m_iPlayerId);
		}
		return;
	}

	if (knownStateVersion != 0 && knownStateVersion != pickup.stateVersion)
	{
		if (isDroppedPickup)
		{
			RejectDropCollectClaim(pickup, player->m_iPlayerId);
		}
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
		if (isDroppedPickup)
		{
			RejectDropCollectClaim(pickup, player->m_iPlayerId);
		}
		return;
	}

	if (isDroppedPickup)
	{
		if (pickup.pendingCollectorPlayerId != -1 && pickup.pendingCollectorPlayerId != player->m_iPlayerId)
		{
			RejectDropCollectClaim(pickup, player->m_iPlayerId);
			return;
		}

		pickup.pendingCollectorPlayerId = player->m_iPlayerId;
		pickup.pendingClaimAtMs = GetServerTimeMs();
		UpdateStateMetadata(pickup);
		BroadcastDropResolve(pickup, CPickupStatePackets::PICKUP_DROP_RESOLVE_ACTION_CLAIM, player->m_iPlayerId);
	}

	pickup.isCollected = true;
	pickup.isSpawned = false;
	pickup.collectorPlayerId = player->m_iPlayerId;
	pickup.pendingCollectorPlayerId = -1;
	pickup.pendingClaimAtMs = 0;
	pickup.collectedAtMs = GetServerTimeMs();

	if (pickup.type == PICKUP_TYPE_JETPACK)
	{
		player->m_bHasJetpack = true;
		pickup.flags &= ~PICKUP_FLAG_RESPAWNABLE;
		pickup.respawnMs = 0;
		CPlayerPackets::PlayerJetpackTransition transitionPacket{};
		transitionPacket.playerid = player->m_iPlayerId;
		transitionPacket.intent = CPlayerPackets::JETPACK_TRANSITION_ACQUIRE;
		transitionPacket.hasJetpack = true;
		CNetwork::SendPacketToAll(CPacketsID::PLAYER_JETPACK_TRANSITION, &transitionPacket, sizeof(transitionPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
		printf("[JetpackTransition][Server][PickupCollect] player=%d pickup=%u type=jetpack hasJetpack=1 nonReusable=1\n", player->m_iPlayerId, pickup.networkId);
	}

	UpdateStateMetadata(pickup);
	PersistCollectibleState(pickup);
	BroadcastDelta(pickup, CPickupStatePackets::PICKUP_ACTION_COLLECT);

	if (isDroppedPickup)
	{
		pickup.flags &= ~PICKUP_FLAG_DROPPED;
		UpdateStateMetadata(pickup);
		BroadcastDropResolve(pickup, CPickupStatePackets::PICKUP_DROP_RESOLVE_ACTION_ACCEPTED, player->m_iPlayerId);
		BroadcastDelta(pickup, CPickupStatePackets::PICKUP_ACTION_REMOVE);
		ms_pickups.erase(it);
		ms_snapshotVersion++;
	}
}

void CPickupManager::ProcessRespawns()
{
	const uint64_t nowMs = GetServerTimeMs();
	for (auto& [id, pickup] : ms_pickups)
	{
		const bool isRespawnable = (pickup.flags & PICKUP_FLAG_RESPAWNABLE) != 0;
		if (!pickup.isCollected || !isRespawnable || pickup.respawnMs == 0)
		{
			continue;
		}

		if (nowMs - pickup.collectedAtMs < pickup.respawnMs)
		{
			continue;
		}

		pickup.isSpawned = true;
		pickup.isCollected = false;
		pickup.collectorPlayerId = -1;
		pickup.collectedAtMs = 0;
		UpdateStateMetadata(pickup);
		PersistCollectibleState(pickup);
		BroadcastDelta(pickup, CPickupStatePackets::PICKUP_ACTION_SPAWN);
	}
}

void CPickupManager::CreateDropsForPlayerDeath(CPlayer* player)
{
	if (!player)
	{
		return;
	}

	const CVector dropPos = player->m_vecPosition;
	constexpr uint8_t droppedFlags = PICKUP_FLAG_DROPPED;

	if (player->m_nMoney > 0)
	{
		int dropMoney = std::clamp(player->m_nMoney / 4, 50, 5000);
		CreatePickup(PICKUP_TYPE_MONEY, 0, dropPos, 1212, 60000, dropMoney, 0, 0, CPickupStatePackets::PICKUP_ORIGIN_DROPPED, droppedFlags);
	}

	const bool isValidWeapon = (player->m_ucCurrentWeapon >= 22 && player->m_ucCurrentWeapon <= 46);
	if (isValidWeapon && player->m_usCurrentAmmo > 0)
	{
		CreatePickup(PICKUP_TYPE_WEAPON, 0, dropPos, player->m_ucCurrentWeapon, 45000, 0, player->m_ucCurrentWeapon, player->m_usCurrentAmmo, CPickupStatePackets::PICKUP_ORIGIN_DROPPED, droppedFlags);
	}

	if (player->m_bHasJetpack)
	{
		CreatePickup(PICKUP_TYPE_JETPACK, 0, dropPos, 370, 90000, 0, 0, 0, CPickupStatePackets::PICKUP_ORIGIN_DROPPED, droppedFlags);
		player->m_bHasJetpack = false;
		CPlayerPackets::PlayerJetpackTransition transitionPacket{};
		transitionPacket.playerid = player->m_iPlayerId;
		transitionPacket.intent = CPlayerPackets::JETPACK_TRANSITION_FORCED_REMOVE;
		transitionPacket.hasJetpack = false;
		CNetwork::SendPacketToAll(CPacketsID::PLAYER_JETPACK_TRANSITION, &transitionPacket, sizeof(transitionPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
		printf("[JetpackTransition][Server][DropOnDeath] player=%d hasJetpack=0\n", player->m_iPlayerId);
	}
}
