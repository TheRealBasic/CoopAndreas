#pragma once
#include <deque>
class CNetworkPlayer
{
public:
	struct InterpSnapshot
	{
		CVector position{};
		CVector velocity{};
		float currentRotation = 0.0f;
		float aimingRotation = 0.0f;
		uint32_t serverSequence = 0;
		uint32_t arrivalTickMs = 0;
	};

	CPlayerPed* m_pPed = nullptr;
	int m_iPlayerId;

	CPackets::PlayerOnFoot m_playerOnFoot{};
	
	signed short m_oShockButtonL;
	signed short m_lShockButtonL;

	CVector* m_vecWaypointPos = nullptr;
	bool m_bWaypointPlaced = false;

	char m_Name[32 + 1] = { 0 };

	CControllerState m_oldControllerState{};
	CControllerState m_newControllerState{};
	CCompressedControllerState m_compressedControllerState{};

	CPackets::PlayerAimSync m_aimSyncData;

	CNetworkPlayerStats m_stats{};
	CPedClothesDesc m_pPedClothesDesc{};
	bool m_bHasBeenConnectedBeforeMe = false;
	std::deque<InterpSnapshot> m_interpSnapshots{};
	uint32_t m_nSnapshotSequence = 0;
	uint32_t m_nInterpCorrectionCount = 0;

	CNetworkPlayer::~CNetworkPlayer();
	CNetworkPlayer::CNetworkPlayer(int id, CVector position);

	void CreatePed(int id, CVector position);
	void DestroyPed();
	void Respawn();
	int GetInternalId();
	std::string GetName();
	char GetWeaponSkill(eWeaponType weaponType);
	void WarpIntoVehicleDriver(CVehicle* vehicle);
	void RemoveFromVehicle(CVehicle* vehicle);
	void WarpIntoVehiclePassenger(CVehicle* vehicle, int seatid);
	void EnterVehiclePassenger(CVehicle* vehicle, int seatid);
	void HandleTask(CPackets::SetPlayerTask& packet);
	void ResetInterpolation(const CVector& position, float currentRotation, float aimingRotation);
};
