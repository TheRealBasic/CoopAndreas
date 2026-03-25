#include "stdafx.h"
#include "CNetworkPed.h"
#include <CTaskSimpleCarSetPedInAsPassenger.h>
#include <CCarEnterExit.h>
#include <CTaskSimpleCarSetPedOut.h>
#include <Hooks/PedHooks.h>

CNetworkPed::CNetworkPed(int pedid, int modelId, ePedType pedType, CVector pos, unsigned char createdBy, char specialModelName[])
{
    m_nModelId = modelId;
    strcpy_s(m_szSpecialModelName, sizeof(m_szSpecialModelName), specialModelName ? specialModelName : "");

    if (modelId >= 290 && modelId <= 299)
        CStreaming::RequestSpecialModel(modelId, specialModelName, 0);
    else
        CStreaming::RequestModel(modelId, 0);

    const uint32_t startTick = GetTickCount();
    while (CStreaming::ms_aInfoForModel[modelId].m_nLoadState != LOADSTATE_LOADED)
    {
        CStreaming::LoadAllRequestedModels(false);
        if (GetTickCount() - startTick > 500)
        {
            return;
        }
        Sleep(10);
    }

    if (pedType == PED_TYPE_COP)
    {
        switch (modelId) 
        {
        case MODEL_LAPD1:
        case MODEL_SFPD1:
        case MODEL_LVPD1:
        case MODEL_LAPDM1:
            modelId = COP_TYPE_CITYCOP;
            break;
        case MODEL_CSHER:
            modelId = COP_TYPE_CSHER;
            break;
        case MODEL_SWAT:
            modelId = COP_TYPE_SWAT1;
            break;
        case MODEL_FBI:
            modelId = COP_TYPE_FBI;
            break;
        case MODEL_ARMY:
            modelId = COP_TYPE_ARMY;
            break;
        }
    }

    if (pedType == PED_TYPE_COP)
    {
        m_pPed = new CCopPed((eCopType)modelId);
    }
    else if (pedType == PED_TYPE_MEDIC || pedType == PED_TYPE_FIREMAN)
    {
        m_pPed = new CEmergencyPed(pedType, modelId);
    }
    else
    {
        m_pPed = new CCivilianPed(pedType, modelId);
    }

    m_pPed->m_nCreatedBy = 2;
    m_pPed->m_pIntelligence->SetPedDecisionMakerType(-1);
    m_pPed->m_pIntelligence->SetSeeingRange(30.0);
    m_pPed->m_pIntelligence->SetHearingRange(30.0);
    m_pPed->m_pIntelligence->m_fDmRadius = 0.0f;
    m_pPed->m_pIntelligence->m_nDmNumPedsToScan = 0;
    
    m_pPed->SetPosn(pos);
    m_pPed->SetOrientation(0.f, 0.f, 0.f);
    CWorld::Add(m_pPed);

    m_nPedId = pedid;
    m_nPedType = pedType;
    m_bSyncing = false;
    m_nCreatedBy = createdBy;
    m_cachedState.position = pos;
    m_cachedState.health = m_pPed ? m_pPed->m_fHealth : 100.0f;
    m_eStreamState = m_pPed ? eStreamState::Streamed : eStreamState::NotStreamed;
}

bool CNetworkPed::StreamIn()
{
    if (m_pPed)
    {
        m_eStreamState = eStreamState::Streamed;
        return true;
    }

    m_eStreamState = eStreamState::StreamingIn;

    if (m_nModelId >= 290 && m_nModelId <= 299)
        CStreaming::RequestSpecialModel(m_nModelId, m_szSpecialModelName, eStreamingFlags::GAME_REQUIRED);
    else
        CStreaming::RequestModel(m_nModelId, eStreamingFlags::GAME_REQUIRED);

    CStreaming::LoadAllRequestedModels(false);

    if (CStreaming::ms_aInfoForModel[m_nModelId].m_nLoadState != LOADSTATE_LOADED)
    {
        m_nFailedStreamInAttempts++;
        m_eStreamState = eStreamState::NotStreamed;
        return false;
    }

    m_pPed = nullptr;
    if (m_nPedType == PED_TYPE_COP)
        m_pPed = new CCopPed((eCopType)m_nModelId);
    else if (m_nPedType == PED_TYPE_MEDIC || m_nPedType == PED_TYPE_FIREMAN)
        m_pPed = new CEmergencyPed(m_nPedType, m_nModelId);
    else
        m_pPed = new CCivilianPed(m_nPedType, m_nModelId);

    if (!m_pPed)
    {
        m_nFailedStreamInAttempts++;
        m_eStreamState = eStreamState::NotStreamed;
        return false;
    }

    m_pPed->m_nCreatedBy = 2;
    m_pPed->m_pIntelligence->SetPedDecisionMakerType(-1);
    m_pPed->m_pIntelligence->SetSeeingRange(30.0);
    m_pPed->m_pIntelligence->SetHearingRange(30.0);
    m_pPed->m_pIntelligence->m_fDmRadius = 0.0f;
    m_pPed->m_pIntelligence->m_nDmNumPedsToScan = 0;
    m_pPed->SetPosn(m_cachedState.position);
    m_pPed->SetOrientation(0.f, 0.f, 0.f);
    CWorld::Add(m_pPed);

    ApplyCachedStateToEntity();

    m_eStreamState = eStreamState::Streamed;
    m_nLastStreamStateChangeTick = GetTickCount();
    return true;
}

void CNetworkPed::StreamOut()
{
    m_eStreamState = eStreamState::StreamingOut;
    CacheAuthoritativeState();

    if (m_pPed && m_pPed->m_matrix && m_pPed->m_matrix->m_pOwner)
    {
        if (m_nBlipHandle != -1)
        {
            CRadar::ClearBlipForEntity(eBlipType::BLIP_CHAR, CPools::GetPedRef(m_pPed));
            m_nBlipHandle = -1;
        }

        if (m_pPed->m_nPedFlags.bInVehicle)
            plugin::Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(CPools::GetPedRef(m_pPed), 0.f, 0.f, 0.f);

        CWorld::Remove(m_pPed);
        m_pPed->Remove();
        delete m_pPed;
    }

    m_pPed = nullptr;
    m_eStreamState = eStreamState::NotStreamed;
    m_nLastStreamStateChangeTick = GetTickCount();
}

void CNetworkPed::CacheAuthoritativeState()
{
    if (!m_pPed || !m_pPed->m_matrix)
        return;

    m_cachedState.position = m_pPed->m_matrix->pos;
    m_cachedState.velocity = m_vecVelocity;
    m_cachedState.aimingRotation = m_fAimingRotation;
    m_cachedState.currentRotation = m_fCurrentRotation;
    m_cachedState.lookDirection = m_fLookDirection;
    m_cachedState.health = m_pPed->m_fHealth;
    m_cachedState.armour = m_pPed->m_fArmour;
}

void CNetworkPed::ApplyCachedStateToEntity()
{
    if (!m_pPed || !m_pPed->m_matrix)
        return;

    m_pPed->m_matrix->pos = m_cachedState.position;
    m_pPed->m_vecMoveSpeed = m_cachedState.velocity;
    m_pPed->m_fAimingRotation = m_fAimingRotation = m_cachedState.aimingRotation;
    m_pPed->m_fCurrentRotation = m_fCurrentRotation = m_cachedState.currentRotation;
    m_pPed->field_73C = m_fLookDirection = m_cachedState.lookDirection;
    m_pPed->m_fHealth = m_fHealth = m_cachedState.health;
    m_pPed->m_fArmour = m_cachedState.armour;
}

CNetworkPed::~CNetworkPed()
{
    if (m_bSyncing)
    {
        CPackets::PedRemove packet{};
        packet.pedid = m_nPedId;
        CNetwork::SendPacket(CPacketsID::PED_REMOVE, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);
    }
	else
	{
		StreamOut();
	}
}

CNetworkPed* CNetworkPed::CreateHosted(CPed* ped)
{
    ped->field_54C += 5000; // m_nTimeTillWeNeedThisPed

    CNetworkPed* networkPed = new CNetworkPed();

    networkPed->m_pPed = ped;
    networkPed->m_nPedId = -1;
    networkPed->m_nCreatedBy = ped->m_nCreatedBy;
    networkPed->m_bSyncing = true;
    networkPed->m_nTempId = CNetworkPedManager::AddToTempList(networkPed);

    CPackets::PedSpawn packet{};
    packet.tempid = networkPed->m_nTempId;
    packet.pedid = networkPed->m_nPedId;
    packet.modelId = ped->m_nModelIndex;
    packet.pos = ped->m_matrix->pos;
    packet.pedType = ped->m_nPedType;
    packet.createdBy = ped->m_nCreatedBy;

    if (packet.modelId >= 290 && packet.modelId <= 299)
    {
        strcpy_s(packet.specialModelName, PedHooks::ms_aszLoadedSpecialModels[packet.modelId - 290]);
        packet.specialModelName[7] = '\0';
    }

    CNetwork::SendPacket(CPacketsID::PED_SPAWN, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);

    return networkPed;
}

void CNetworkPed::WarpIntoVehicleDriver(CVehicle* vehicle)
{
    assert(m_pPed != nullptr);

    if (!CUtil::IsValidEntityPtr(vehicle) || !CUtil::IsValidEntityPtr(m_pPed))
    {
        return;
    }

    if (m_pPed->m_nPedFlags.bInVehicle && m_pPed->m_pVehicle)
    {
        RemoveFromVehicle(m_pPed->m_pVehicle);
    }

    m_pPed->m_pIntelligence->FlushImmediately(false);

    if (!m_bSyncing)
    {
        m_pPed->m_nPedFlags.CantBeKnockedOffBike = 1; // 1 - never
    }

    auto task = CTaskSimpleCarSetPedInAsDriver(vehicle, nullptr);
    task.m_bWarpingInToCar = true;
    task.ProcessPed(m_pPed);
}

void CNetworkPed::WarpIntoVehiclePassenger(CVehicle* vehicle, int seatid)
{
    assert(m_pPed != nullptr);

    if (!CUtil::IsValidEntityPtr(vehicle) || !CUtil::IsValidEntityPtr(m_pPed))
    {
        return;
    }

    if (m_pPed->m_nPedFlags.bInVehicle && m_pPed->m_pVehicle)
    {
        RemoveFromVehicle(m_pPed->m_pVehicle);
    }

    m_pPed->m_pIntelligence->FlushImmediately(false);

    if (!m_bSyncing)
    {
        m_pPed->m_nPedFlags.CantBeKnockedOffBike = 1; // 1 - never
    }

    int doorId = CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(vehicle, seatid);
    auto task = CTaskSimpleCarSetPedInAsPassenger(vehicle, doorId, nullptr);
    task.m_bWarpingInToCar = true;
    task.ProcessPed(m_pPed);
}

void CNetworkPed::RemoveFromVehicle(CVehicle* vehicle)
{
    assert(m_pPed != nullptr);

    if (!CUtil::IsValidEntityPtr(vehicle) || !CUtil::IsValidEntityPtr(m_pPed))
    {
        return;
    }

    m_pPed->m_pIntelligence->m_TaskMgr.SetTask(nullptr, TASK_PRIMARY_PRIMARY, false);

    if (!m_bSyncing)
    {
        m_pPed->m_nPedFlags.CantBeKnockedOffBike = 2; // 2 - normal
    }

    auto task = CTaskSimpleCarSetPedOut(vehicle, 1, false);
    task.m_bWarpingOutOfCar = true;
    task.ProcessPed(m_pPed);

    m_pPed->m_pIntelligence->FlushImmediately(true); // create a default primary task (fix bug)
}

void CNetworkPed::ClaimOnRelease()
{
    if (m_bClaimOnRelease || m_bSyncing)
        return;

    CPackets::PedClaimOnRelease packet{};
    packet.pedid = m_nPedId;
    CNetwork::SendPacket(CPacketsID::PED_CLAIM_ON_RELEASE, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);

    m_bClaimOnRelease = true;
}

void CNetworkPed::CancelClaim()
{
    if (!m_bClaimOnRelease || m_bSyncing)
        return;

    CPackets::PedCancelClaim packet{};
    packet.pedid = m_nPedId;
    CNetwork::SendPacket(CPacketsID::PED_CANCEL_CLAIM, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);

    m_bClaimOnRelease = false;
}
