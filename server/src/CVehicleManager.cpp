
#include "CVehicle.h"
#include "CVehicleManager.h"
#include <chrono>
#include "persistence/SnapshotPersistence.h"

std::vector<CVehicle*> CVehicleManager::m_pVehicles;
static uint32_t s_trailerLinkVersionCounter = 0;

void CVehicleManager::Add(CVehicle* vehicle)
{
	m_pVehicles.push_back(vehicle);
	CSnapshotPersistence::MarkDirty("vehicle_add");
}

void CVehicleManager::Remove(CVehicle* vehicle)
{
	auto it = std::find(m_pVehicles.begin(), m_pVehicles.end(), vehicle);
	if (it != m_pVehicles.end())
	{
		m_pVehicles.erase(it);
		CSnapshotPersistence::MarkDirty("vehicle_remove");
	}
}

CVehicle* CVehicleManager::GetVehicle(int vehicleid)
{
	for (int i = 0; i != m_pVehicles.size(); i++)
	{
		if (m_pVehicles[i]->m_nVehicleId == vehicleid)
		{
			return m_pVehicles[i];
		}
	}
	return nullptr;
}

bool CVehicleManager::IsNetworkVehicleActive(CVehicle* vehicle)
{
	return vehicle != nullptr && vehicle->m_nVehicleId >= 0 && vehicle->m_pSyncer != nullptr;
}

int CVehicleManager::GetFreeID()
{
	for (int i = 0; i < 200; i++)
	{
		if (CVehicleManager::GetVehicle(i) == nullptr)
			return i;
	}

	return -1;
}

uint64_t CVehicleManager::GetAuthoritativeTimestampMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

bool CVehicleManager::ProcessTrailerLinkEvent(
	CPlayer* sourcePlayer,
	int tractorVehicleId,
	int trailerVehicleId,
	uint8_t attachState,
	uint8_t detachReason,
	uint64_t& timestampMs,
	uint32_t& linkVersion,
	bool validateAuthority)
{
	if (!sourcePlayer)
		return false;

	CVehicle* tractor = CVehicleManager::GetVehicle(tractorVehicleId);
	CVehicle* trailer = CVehicleManager::GetVehicle(trailerVehicleId);
	if (!CVehicleManager::IsNetworkVehicleActive(tractor) || !CVehicleManager::IsNetworkVehicleActive(trailer))
		return false;

	if (tractor == trailer)
		return false;

	if (validateAuthority && sourcePlayer != tractor->m_pSyncer && sourcePlayer != trailer->m_pSyncer)
		return false;

	attachState = attachState ? 1 : 0;
	detachReason = attachState ? CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_NONE : detachReason;

	s_trailerLinkVersionCounter = std::max<uint32_t>(s_trailerLinkVersionCounter, std::max(tractor->m_nTrailerLinkVersion, trailer->m_nTrailerLinkVersion));
	linkVersion = ++s_trailerLinkVersionCounter;
	timestampMs = CVehicleManager::GetAuthoritativeTimestampMs();

	auto clearCounterpartLink = [timestampMs, linkVersion](CVehicle* counterpart)
	{
		if (!counterpart)
			return;
		counterpart->m_nLinkedTrailerId = -1;
		counterpart->m_nLinkedTractorId = -1;
		counterpart->m_nTrailerAttachState = 0;
		counterpart->m_nTrailerDetachReason = CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_FORCE;
		counterpart->m_nTrailerLinkTimestampMs = timestampMs;
		counterpart->m_nTrailerLinkVersion = linkVersion;
	};

	if (attachState)
	{
		if (tractor->m_nLinkedTrailerId >= 0 && tractor->m_nLinkedTrailerId != trailerVehicleId)
			clearCounterpartLink(CVehicleManager::GetVehicle(tractor->m_nLinkedTrailerId));
		if (trailer->m_nLinkedTractorId >= 0 && trailer->m_nLinkedTractorId != tractorVehicleId)
			clearCounterpartLink(CVehicleManager::GetVehicle(trailer->m_nLinkedTractorId));
	}

	tractor->m_nLinkedTrailerId = attachState ? trailerVehicleId : -1;
	tractor->m_nLinkedTractorId = -1;
	tractor->m_nTrailerAttachState = attachState;
	tractor->m_nTrailerDetachReason = detachReason;
	tractor->m_nTrailerLinkTimestampMs = timestampMs;
	tractor->m_nTrailerLinkVersion = linkVersion;

	trailer->m_nLinkedTractorId = attachState ? tractorVehicleId : -1;
	trailer->m_nLinkedTrailerId = -1;
	trailer->m_nTrailerAttachState = attachState;
	trailer->m_nTrailerDetachReason = detachReason;
	trailer->m_nTrailerLinkTimestampMs = timestampMs;
	trailer->m_nTrailerLinkVersion = linkVersion;

	return true;
}

void CVehicleManager::RemoveAllHostedAndNotify(CPlayer* player)
{
	CVehiclePackets::VehicleRemove packet{};

	for (auto it = CVehicleManager::m_pVehicles.begin(); it != CVehicleManager::m_pVehicles.end();)
	{
		if ((*it)->m_pSyncer == player)
		{
			uint64_t ts = 0;
			uint32_t version = 0;
			CVehiclePackets::VehicleTrailerLinkSync trailerLinkPacket{};
			if ((*it)->m_nLinkedTractorId >= 0)
			{
				CVehicleManager::ProcessTrailerLinkEvent(
					player,
					(*it)->m_nLinkedTractorId,
					(*it)->m_nVehicleId,
					0,
					CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_ENTITY_REMOVED,
					ts,
					version,
					false);
				trailerLinkPacket.tractorVehicleId = (*it)->m_nLinkedTractorId;
				trailerLinkPacket.trailerVehicleId = (*it)->m_nVehicleId;
				trailerLinkPacket.attachState = 0;
				trailerLinkPacket.detachReason = CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_ENTITY_REMOVED;
				trailerLinkPacket.timestampMs = ts;
				trailerLinkPacket.linkVersion = version;
				CNetwork::SendPacketToAll(CPacketsID::VEHICLE_TRAILER_LINK_SYNC, &trailerLinkPacket, sizeof(trailerLinkPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
			}
			if ((*it)->m_nLinkedTrailerId >= 0)
			{
				CVehicleManager::ProcessTrailerLinkEvent(
					player,
					(*it)->m_nVehicleId,
					(*it)->m_nLinkedTrailerId,
					0,
					CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_ENTITY_REMOVED,
					ts,
					version,
					false);
				trailerLinkPacket.tractorVehicleId = (*it)->m_nVehicleId;
				trailerLinkPacket.trailerVehicleId = (*it)->m_nLinkedTrailerId;
				trailerLinkPacket.attachState = 0;
				trailerLinkPacket.detachReason = CVehiclePackets::VehicleTrailerLinkSync::DETACH_REASON_ENTITY_REMOVED;
				trailerLinkPacket.timestampMs = ts;
				trailerLinkPacket.linkVersion = version;
				CNetwork::SendPacketToAll(CPacketsID::VEHICLE_TRAILER_LINK_SYNC, &trailerLinkPacket, sizeof(trailerLinkPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
			}

			packet.vehicleid = (*it)->m_nVehicleId;
			CNetwork::SendPacketToAll(CPacketsID::VEHICLE_REMOVE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, player->m_pPeer);
			delete *it;
			it = CVehicleManager::m_pVehicles.erase(it);
			CSnapshotPersistence::MarkDirty("vehicle_remove_hosted");
		}
		else
		{
			++it;
		}
	}
}
