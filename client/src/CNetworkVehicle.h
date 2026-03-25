#pragma once
class CNetworkVehicle
{
private:
	CNetworkVehicle() {}
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
		CVector roll{};
		CVector up{};
		CVector velocity{};
		CVector turnSpeed{};
		float health = 1000.0f;
		unsigned char color1 = 0;
		unsigned char color2 = 0;
		char paintjob = -1;
		eDoorLock locked = DOORLOCK_UNLOCKED;
		unsigned char driverPlayerId = 255;
	};

	struct PassengerSeatState
	{
		unsigned char gamepadFlags = 0;
		signed char radioStation = -1;
		unsigned char radioState = 0;
		uint32_t lastRadioApplyTick = 0;
	};

	int m_nVehicleId = -1;
	CVehicle* m_pVehicle = nullptr;
	int m_nModelId = 0;
	char m_nPaintJob = -1;
	bool m_bSyncing = false;
	unsigned char m_nTempId = 255;
	unsigned char m_nCreatedBy;
	int m_nBlipHandle = -1;
	bool m_bMissionCritical = false;
	eStreamState m_eStreamState = eStreamState::NotStreamed;
	CachedState m_cachedState{};
	uint32_t m_nLastStreamStateChangeTick = 0;
	uint32_t m_nLastStreamDistanceFailTick = 0;
	int m_nFailedStreamInAttempts = 0;
	PassengerSeatState m_passengerSeatState[8]{};

	~CNetworkVehicle();
	CNetworkVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2, unsigned char createdBy);
	bool CreateVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2);
	bool StreamIn();
	void StreamOut();
	void CacheAuthoritativeState();
	void ApplyCachedStateToEntity();
	void SetPassengerSeatState(uint8_t seatId, const PassengerSeatState& state, bool applyToAudio);
	void ClearPassengerSeatState(uint8_t seatId);
	bool HasDriver();
	
	static CNetworkVehicle* CreateHosted(CVehicle* vehicle);
};
