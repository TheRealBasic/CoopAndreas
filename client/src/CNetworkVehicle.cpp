#include "stdafx.h"
#include "CNetworkVehicle.h"

CNetworkVehicle::CNetworkVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2, unsigned char createdBy)
{
    if (auto vehicle = CNetworkVehicleManager::GetVehicle(vehicleid))
    {
        if (vehicle->m_pVehicle && CUtil::IsValidEntityPtr(vehicle->m_pVehicle))
        {
            CWorld::Remove(vehicle->m_pVehicle);
            delete vehicle->m_pVehicle;
        }
        CNetworkVehicleManager::Remove(vehicle);
    }

    m_nVehicleId = vehicleid;
    m_bSyncing = false;
    m_nTempId = 255;
    m_nModelId = modelid;
    m_nCreatedBy = createdBy;
    m_cachedState.position = pos;
    m_cachedState.health = 1000.0f;
    m_cachedState.color1 = color1;
    m_cachedState.color2 = color2;

    if (CreateVehicle(vehicleid, modelid, pos, rotation, color1, color2))
    {
        m_eStreamState = eStreamState::Streamed;
        ApplyCachedStateToEntity();
        ResetInterpolationFromCached();
    }

}

bool CNetworkVehicle::CreateVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2)
{
    unsigned char oldFlags = CStreaming::ms_aInfoForModel[modelid].m_nFlags;
    CStreaming::RequestModel(modelid, GAME_REQUIRED | eStreamingFlags::PRIORITY_REQUEST);

    const uint32_t startTick = GetTickCount();
    while (CStreaming::ms_aInfoForModel[modelid].m_nLoadState != LOADSTATE_LOADED)
    {
        CStreaming::LoadAllRequestedModels(false);
        if (GetTickCount() - startTick > 500)
        {
            return false;
        }
        Sleep(10);
    }

    if (!(oldFlags & GAME_REQUIRED))
    {
        CStreaming::SetModelIsDeletable(modelid);
        CStreaming::SetModelTxdIsDeletable(modelid);
    }

    switch (((CVehicleModelInfo*)CModelInfo::ms_modelInfoPtrs[modelid])->m_nVehicleType)
    {
    case VEHICLE_MTRUCK:
        m_pVehicle = new CMonsterTruck(modelid, MISSION_VEHICLE); break;

    case VEHICLE_QUAD:
        m_pVehicle = new CQuadBike(modelid, MISSION_VEHICLE); break;

    case VEHICLE_HELI:
        m_pVehicle = new CHeli(modelid, MISSION_VEHICLE); break;

    case VEHICLE_PLANE:
        m_pVehicle = new CPlane(modelid, MISSION_VEHICLE); break;

    case VEHICLE_BIKE:
        m_pVehicle = new CBike(modelid, MISSION_VEHICLE);
        ((CBike*)m_pVehicle)->m_nDamageFlags |= 0x10; break;

    case VEHICLE_BMX:
        m_pVehicle = new CBmx(modelid, MISSION_VEHICLE);
        ((CBmx*)m_pVehicle)->m_nDamageFlags |= 0x10; break;

    case VEHICLE_TRAILER:
        m_pVehicle = new CTrailer(modelid, MISSION_VEHICLE); break;

    case VEHICLE_BOAT:
        m_pVehicle = new CBoat(modelid, MISSION_VEHICLE); break;

    case VEHICLE_TRAIN:
        return false;

    default:
        m_pVehicle = new CAutomobile(modelid, MISSION_VEHICLE, true); break;
    }

    if (!m_pVehicle)
        return false;

    m_pVehicle->SetPosn(pos);
    m_pVehicle->SetOrientation(0.0f, 0.0f, rotation);
    m_pVehicle->m_nStatus = 4;
    m_pVehicle->m_eDoorLock = DOORLOCK_UNLOCKED;
    m_pVehicle->m_nPrimaryColor = color1;
    m_pVehicle->m_nSecondaryColor = color2;
    CWorld::Add(m_pVehicle);

	return true;
}

bool CNetworkVehicle::StreamIn()
{
    m_eStreamState = eStreamState::StreamingIn;
    if (!CreateVehicle(m_nVehicleId, m_nModelId, m_cachedState.position, 0.0f, m_cachedState.color1, m_cachedState.color2))
    {
        m_nFailedStreamInAttempts++;
        m_eStreamState = eStreamState::NotStreamed;
        return false;
    }

    ApplyCachedStateToEntity();
    ResetInterpolationFromCached();
    m_eStreamState = eStreamState::Streamed;
    m_nLastStreamStateChangeTick = GetTickCount();
    return true;
}

void CNetworkVehicle::StreamOut()
{
    m_eStreamState = eStreamState::StreamingOut;
    CacheAuthoritativeState();

    if (m_pVehicle && CUtil::IsValidEntityPtr(m_pVehicle))
    {
        if (m_nBlipHandle != -1)
        {
            CRadar::ClearBlipForEntity(eBlipType::BLIP_CAR, CPools::GetVehicleRef(m_pVehicle));
            m_nBlipHandle = -1;
        }

        plugin::Command<Commands::DELETE_CAR>(CPools::GetVehicleRef(m_pVehicle));
    }

    m_pVehicle = nullptr;
    m_eStreamState = eStreamState::NotStreamed;
    m_nLastStreamStateChangeTick = GetTickCount();
}

void CNetworkVehicle::CacheAuthoritativeState()
{
    if (!m_pVehicle || !CUtil::IsValidEntityPtr(m_pVehicle) || !m_pVehicle->m_matrix)
        return;

    m_cachedState.position = m_pVehicle->m_matrix->pos;
    m_cachedState.roll = m_pVehicle->m_matrix->right;
    m_cachedState.up = m_pVehicle->m_matrix->up;
    m_cachedState.velocity = m_pVehicle->m_vecMoveSpeed;
    m_cachedState.turnSpeed = m_pVehicle->m_vecTurnSpeed;
    m_cachedState.health = m_pVehicle->m_fHealth;
    m_cachedState.color1 = m_pVehicle->m_nPrimaryColor;
    m_cachedState.color2 = m_pVehicle->m_nSecondaryColor;
    m_cachedState.paintjob = m_nPaintJob;
    m_cachedState.locked = m_pVehicle->m_eDoorLock;
}

void CNetworkVehicle::ApplyCachedStateToEntity()
{
    if (!m_pVehicle || !CUtil::IsValidEntityPtr(m_pVehicle) || !m_pVehicle->m_matrix)
        return;

    if (m_cachedState.roll.Magnitude() < 0.001f || m_cachedState.up.Magnitude() < 0.001f)
    {
        m_cachedState.roll = m_pVehicle->m_matrix->right;
        m_cachedState.up = m_pVehicle->m_matrix->up;
    }

    m_pVehicle->m_matrix->pos = m_cachedState.position;
    m_pVehicle->m_matrix->right = m_cachedState.roll;
    m_pVehicle->m_matrix->up = m_cachedState.up;
    m_pVehicle->m_vecMoveSpeed = m_cachedState.velocity;
    m_pVehicle->m_vecTurnSpeed = m_cachedState.turnSpeed;
    m_pVehicle->m_fHealth = m_cachedState.health;
    m_pVehicle->m_nPrimaryColor = m_cachedState.color1;
    m_pVehicle->m_nSecondaryColor = m_cachedState.color2;
    m_pVehicle->m_eDoorLock = m_cachedState.locked;
    if (m_nPaintJob != m_cachedState.paintjob)
    {
        m_nPaintJob = m_cachedState.paintjob;
        m_pVehicle->SetRemap(m_nPaintJob);
    }
}

void CNetworkVehicle::ResetInterpolationFromCached()
{
    m_interpSnapshots.clear();
    InterpSnapshot snapshot{};
    snapshot.position = m_cachedState.position;
    snapshot.roll = m_cachedState.roll;
    snapshot.up = m_cachedState.up;
    snapshot.velocity = m_cachedState.velocity;
    snapshot.turnSpeed = m_cachedState.turnSpeed;
    snapshot.health = m_cachedState.health;
    snapshot.arrivalTickMs = GetTickCount();
    snapshot.serverSequence = ++m_nSnapshotSequence;
    m_interpSnapshots.push_back(snapshot);
}

void CNetworkVehicle::SetPassengerSeatState(uint8_t seatId, const PassengerSeatState& state, bool applyToAudio)
{
    if (seatId >= 8)
        return;

    m_passengerSeatState[seatId] = state;

    if (!applyToAudio || !m_pVehicle || !CUtil::IsValidEntityPtr(m_pVehicle))
        return;

    // We do not force-write game internals here. Instead we trigger the native vehicle
    // audio update path with the latest networked passenger radio state cached.
    const uint32_t now = GetTickCount();
    if (now - m_passengerSeatState[seatId].lastRadioApplyTick >= 250)
    {
        plugin::CallMethod<0x502280, CAEVehicleAudioEntity*>(&m_pVehicle->m_vehicleAudio);
        m_passengerSeatState[seatId].lastRadioApplyTick = now;
    }
}

void CNetworkVehicle::ClearPassengerSeatState(uint8_t seatId)
{
    if (seatId >= 8)
        return;
    m_passengerSeatState[seatId] = PassengerSeatState{};
}

CNetworkVehicle::~CNetworkVehicle()
{
    if (m_bSyncing)
    {
        CPackets::VehicleRemove vehicleRemovePacket{};
        vehicleRemovePacket.vehicleid = m_nVehicleId;
        CNetwork::SendPacket(CPacketsID::VEHICLE_REMOVE, &vehicleRemovePacket, sizeof vehicleRemovePacket, ENET_PACKET_FLAG_RELIABLE);
    }
	else
	{
		StreamOut();
	}
}

bool CNetworkVehicle::HasDriver()
{
    if (m_pVehicle == nullptr)
        return false;

    return m_pVehicle->m_pDriver != nullptr;
}

CNetworkVehicle* CNetworkVehicle::CreateHosted(CVehicle* vehicle)
{
    vehicle->m_nTimeTillWeNeedThisCar += 5000;

    CNetworkVehicle* networkVehicle = new CNetworkVehicle();

    networkVehicle->m_pVehicle = vehicle;
    networkVehicle->m_nVehicleId = -1;
    networkVehicle->m_bSyncing = true;
    networkVehicle->m_nModelId = vehicle->m_nModelIndex;
    networkVehicle->m_nPaintJob = (char)vehicle->m_nRemapTxd;
    networkVehicle->m_nTempId = CNetworkVehicleManager::AddToTempList(networkVehicle);
    networkVehicle->m_nCreatedBy = vehicle->m_nCreatedBy;

    CPackets::VehicleSpawn vehicleSpawnPacket{};
    vehicleSpawnPacket.vehicleid = -1;
    vehicleSpawnPacket.tempid = networkVehicle->m_nTempId;
    vehicleSpawnPacket.modelid = vehicle->m_nModelIndex;
    vehicleSpawnPacket.pos = vehicle->m_matrix->pos;
    vehicleSpawnPacket.rot = vehicle->GetHeading();
    vehicleSpawnPacket.color1 = vehicle->m_nPrimaryColor;
    vehicleSpawnPacket.color2 = vehicle->m_nSecondaryColor;
    vehicleSpawnPacket.createdBy = vehicle->m_nCreatedBy;
    CNetwork::SendPacket(CPacketsID::VEHICLE_SPAWN, &vehicleSpawnPacket, sizeof vehicleSpawnPacket, ENET_PACKET_FLAG_RELIABLE);

    return networkVehicle;
}
