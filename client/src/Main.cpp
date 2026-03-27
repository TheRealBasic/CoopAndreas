#include "stdafx.h"
#include "CDXFont.h"
#include <Hooks/WorldHooks.h>
#include "CCutsceneMgr.h"
#include <CDiscordRPCMgr.h>
#include <CCarEnterExit.h>
#include <CTaskSimpleCarSetPedInAsPassenger.h>
#include <CTaskSimpleCarSetPedOut.h>
#include <CNetworkPlayerList.h>
#include <CFireManager.h>
#include <semver.h>
#include <COpCodeSync.h>
#include <CMissionSyncState.h>
#include <CStatsSync.h>
#include <CNetworkCheckpoint.h>
#include <CEntryExitManager.h>
#include <CEntryExitMarkerSync.h>
#include <CNetworkStaticBlip.h>
#include <CNetworkAnimQueue.h>
#include <CNetworkPickupManager.h>
#include <game_sa/CTagManager.h>
#include <CPedPlacement.h>
#include <CGeneral.h>
#include "CMainUpdateCoordinators.h"

class CoopAndreas {
public:
    CoopAndreas() {
		Events::shutdownRwEvent += []
			{
				
			};
		Events::gameProcessEvent.before += []
			{
				CNetworkIngestCoordinator::Process();
			};
		Events::gameProcessEvent += []
			{
				CNetworkAnimQueue::Process();
				CDiscordRPCMgr::Update();
				CDebugVehicleSpawner::Process();
				
				if (CNetwork::m_bConnected)
				{
					CSyncEmitCoordinator::ProcessConnectedFrame();
					CRemoteEntityProcessingCoordinator::Process();
				}
			};
		Events::drawBlipsEvent += []
			{
				CNetworkPlayerMapPin::Process();
				CNetworkPlayerWaypoint::Process();
			};
		Events::drawingEvent += []
			{
				CHudDebugDrawCoordinator::Process();
			};
		CCore::Init();
		
	};
} CoopAndreasPlugin;
