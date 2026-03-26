#pragma once

#ifndef _CVEHICLE_H_
	#define _CVEHICLE_H_
#include <iostream>
#include <vector>
#include <algorithm>

#include "CVector.h"
#include "CPlayerManager.h"

class CVehicle
{
	public:
		struct PassengerSeatState
		{
			uint8_t gamepadFlags = 0;
			int8_t radioStation = -1;
			uint8_t radioState = 0;
		};
		struct HydraulicsSyncState
		{
			uint8_t controlState = 0;
			uint8_t transitionMask = 0;
			uint16_t transitionSequence = 0;
		};

		CVehicle(int vehicleid, unsigned short model, CVector pos, float rot);
		
		int m_nVehicleId;
		CPlayer* m_pSyncer = nullptr;
		unsigned short m_nModelId;
		CVector m_vecPosition;
		CVector m_vecRotation;
		CVector m_vecVelocity;
		CPlayer* m_pPlayers[8] = { nullptr };
		unsigned char m_nPrimaryColor;
		unsigned char m_nSecondaryColor;
		unsigned char m_damageManager_padding[23] = { 0 };
		std::vector<int> m_pComponents;
		uint8_t m_nCreatedBy;
		bool m_bUsedByPed = false;
		PassengerSeatState m_passengerSeatState[8]{};
		HydraulicsSyncState m_hydraulicsState{};

		void ReassignSyncer(CPlayer* newSyncer);
		void SetOccupant(uint8_t seatid, CPlayer* player);
		void ClearPassengerSeatState(uint8_t seatid);

		~CVehicle() {}
};


#endif
