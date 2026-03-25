#pragma once
#include <deque>
class CNetworkPed
{
private:
	CNetworkPed() {}
public:
	enum class eStreamState : unsigned char
	{
		NotStreamed,
		StreamingIn,
		Streamed,
		StreamingOut
	};

	struct CachedState
	{
		CVector position{};
		CVector velocity{};
		float aimingRotation = 0.0f;
		float currentRotation = 0.0f;
		int lookDirection = 0;
		float health = 100.0f;
		float armour = 0.0f;
		unsigned char weapon = 0;
		unsigned short ammo = 0;
		unsigned char seat = 255;
		int occupiedVehicleId = -1;
	};
	struct InterpSnapshot
	{
		CVector position{};
		CVector velocity{};
		float aimingRotation = 0.0f;
		float currentRotation = 0.0f;
		int lookDirection = 0;
		float health = 100.0f;
		float armour = 0.0f;
		uint32_t serverSequence = 0;
		uint32_t arrivalTickMs = 0;
	};

	int m_nPedId = -1;
	CPed* m_pPed = nullptr;
	bool m_bSyncing = false;
	unsigned char m_nTempId = 255;
	ePedType m_nPedType;
	unsigned char m_nCreatedBy;
	CVector m_vecVelocity{0.0f, 0.0f, 0.0f};
	float m_fAimingRotation = 0.0f;
	float m_fCurrentRotation = 0.0f;
	int m_fLookDirection{};
	eMoveState m_nMoveState = eMoveState::PEDMOVE_NONE;
	float m_fMoveBlendRatio = 0.0f;
	CAutoPilot m_autoPilot;
	float m_fGasPedal = 0.0f;
	float m_fBreakPedal = 0.0f;
	float m_fSteerAngle = 0.0f;
	float m_fHealth = 100.0f;
	int m_nBlipHandle = -1;
	bool m_bClaimOnRelease = false;
	bool m_bMissionCritical = false;
	eStreamState m_eStreamState = eStreamState::NotStreamed;
	CachedState m_cachedState{};
	uint32_t m_nLastStreamStateChangeTick = 0;
	int m_nModelId = 0;
	char m_szSpecialModelName[8]{};
	int m_nFailedStreamInAttempts = 0;
	std::deque<InterpSnapshot> m_interpSnapshots{};
	uint32_t m_nSnapshotSequence = 0;
	uint32_t m_nInterpCorrectionCount = 0;

	static CNetworkPed* CreateHosted(CPed* ped);
	bool StreamIn();
	void StreamOut();
	void CacheAuthoritativeState();
	void ApplyCachedStateToEntity();
	void ResetInterpolationFromCached();
	void WarpIntoVehicleDriver(CVehicle* vehicle);
	void WarpIntoVehiclePassenger(CVehicle* vehicle, int seatid);
	void RemoveFromVehicle(CVehicle* vehicle);
	void ClaimOnRelease();
	void CancelClaim();
	CNetworkPed(int pedid, int modelId, ePedType pedType, CVector pos, unsigned char createdBy, char specialModelName[]);
	~CNetworkPed();
};
