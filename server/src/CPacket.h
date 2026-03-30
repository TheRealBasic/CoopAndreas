#pragma once

#ifndef _CPACKET_H_
	#define _CPACKET_H_

#include "CVector.h"
#include <cstdint>

#include "PacketRegistry.h"


class COpCodePackets
{
public:
	#pragma pack(1)
	struct OpCodeSyncHeader
	{
		uint16_t opcode;
		uint16_t intParamCount;
		uint16_t stringParamCount;
	};
	#pragma pack()
};


class CPickupStatePackets
{
public:
	#pragma pack(1)
	enum ePickupAction : uint8_t
	{
		PICKUP_ACTION_SPAWN = 0,
		PICKUP_ACTION_COLLECT = 1,
		PICKUP_ACTION_REMOVE = 2
	};

	enum ePickupDropResolveAction : uint8_t
	{
		PICKUP_DROP_RESOLVE_ACTION_CLAIM = 0,
		PICKUP_DROP_RESOLVE_ACTION_ACCEPTED = 1,
		PICKUP_DROP_RESOLVE_ACTION_REJECTED = 2
	};

	enum ePickupOrigin : uint8_t
	{
		PICKUP_ORIGIN_COLLECTIBLE = 0,
		PICKUP_ORIGIN_STATIC = 1,
		PICKUP_ORIGIN_DROPPED = 2
	};

	struct PickupSnapshotBegin
	{
		uint32_t snapshotVersion;
		uint32_t pickupCount;
	};

	struct PickupSnapshotEntry
	{
		uint32_t networkId;
		uint8_t type;
		uint8_t category;
		uint32_t worldCollectibleId;
		uint8_t origin;
		uint8_t flags;
		CVector position;
		int modelId;
		int amount;
		uint8_t weaponId;
		uint16_t weaponAmmo;
		uint32_t respawnMs;
		bool isSpawned;
		bool isCollected;
		int collectorPlayerId;
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};

	struct PickupSnapshotEnd
	{
		uint32_t snapshotVersion;
	};

	struct PickupSnapshotRequest
	{
		int requesterPlayerId;
		uint32_t clientSnapshotVersion;
		uint8_t reason;
	};

	struct PickupCollectRequest
	{
		uint32_t networkId;
		int playerId;
		CVector playerPosition;
		uint32_t knownStateVersion;
	};

	struct PickupStateDelta
	{
		uint32_t networkId;
		uint8_t action;
		bool isSpawned;
		bool isCollected;
		int collectorPlayerId;
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};

	struct PickupDropCreate
	{
		PickupSnapshotEntry pickup;
	};

	struct PickupDropResolve
	{
		uint32_t networkId;
		uint8_t action;
		int resolverPlayerId;
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};
	#pragma pack()
};

class CTrailerLinkPackets
{
public:
	#pragma pack(1)
	enum eTrailerDetachReason : uint8_t
	{
		TRAILER_DETACH_REASON_NONE = 0,
		TRAILER_DETACH_REASON_MANUAL = 1,
		TRAILER_DETACH_REASON_FORCE = 2,
		TRAILER_DETACH_REASON_ENTITY_REMOVED = 3
	};

	struct VehicleTrailerLinkSync
	{
		int tractorVehicleId;
		int trailerVehicleId;
		uint8_t attachState;
		uint8_t detachReason;
		uint64_t timestampMs;
		uint32_t linkVersion;
	};
	#pragma pack()
};

class CPropertyStatePackets
{
public:
	#pragma pack(1)
	enum ePropertyStateAction : uint8_t
	{
		PROPERTY_STATE_ACTION_UPSERT = 0,
		PROPERTY_STATE_ACTION_REMOVE = 1
	};

	struct PropertyStateSnapshotBegin
	{
		uint32_t snapshotVersion;
		uint16_t propertyCount;
	};

	struct PropertyStateSnapshotEntry
	{
		uint16_t propertyId;
		int ownerPlayerId;
		uint8_t unlocked;
		uint8_t linkedPickupActive;
		uint8_t linkedInteriorUnlocked;
		uint8_t currArea;
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};

	struct PropertyStateSnapshotEnd
	{
		uint32_t snapshotVersion;
	};

	struct PropertyStateDelta : public PropertyStateSnapshotEntry
	{
		uint8_t action;
	};
	#pragma pack()
};

class CSubmissionMissionPackets
{
public:
	#pragma pack(1)
	enum eSubmissionMissionAction : uint8_t
	{
		SUBMISSION_MISSION_ACTION_UPSERT = 0,
		SUBMISSION_MISSION_ACTION_CLEAR = 1
	};

	enum eSubmissionMissionType : uint8_t
	{
		SUBMISSION_MISSION_TAXI = 0,
		SUBMISSION_MISSION_FIREFIGHTER = 1,
		SUBMISSION_MISSION_VIGILANTE = 2,
		SUBMISSION_MISSION_PARAMEDIC = 3,
		SUBMISSION_MISSION_PIMP = 4,
		SUBMISSION_MISSION_FREIGHT_TRAIN = 5,
		SUBMISSION_MISSION_TYPE_MAX = 6
	};

	struct SubmissionMissionSnapshotBegin
	{
		uint32_t snapshotVersion;
		uint8_t submissionCount;
	};

	struct SubmissionMissionSnapshotEntry
	{
		uint8_t submissionType;
		uint8_t active;
		uint8_t level;
		uint8_t stage;
		uint16_t progress;
		int32_t timerMs;
		int32_t score;
		int32_t rewardCash;
		uint8_t outcome;
		uint8_t participantCount;
		uint8_t currArea;
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};

	struct SubmissionMissionSnapshotEnd
	{
		uint32_t snapshotVersion;
	};

	struct SubmissionMissionStateDelta : public SubmissionMissionSnapshotEntry
	{
		uint8_t action;
	};
	#pragma pack()
};

class CMissionRuntimePackets
{
public:
	#pragma pack(1)
	enum eMissionRuntimeState : uint8_t
	{
		MISSION_RUNTIME_IDLE = 0,
		MISSION_RUNTIME_STARTING = 1,
		MISSION_RUNTIME_ACTIVE = 2,
		MISSION_RUNTIME_PASS_PENDING = 3,
		MISSION_RUNTIME_FAIL_PENDING = 4,
		MISSION_RUNTIME_COMPLETED = 5
	};

	enum eMissionTerminalReasonCode : uint8_t
	{
		MISSION_TERMINAL_REASON_NONE = 0,
		MISSION_TERMINAL_REASON_PASS = 1,
		MISSION_TERMINAL_REASON_FAIL = 2,
		MISSION_TERMINAL_REASON_ON_MISSION_CLEARED = 3
	};

	struct OnMissionFlagSync
	{
		uint8_t bOnMission : 1;
		uint32_t missionEpoch;
	};

	struct MissionFlowSync
	{
		uint8_t eventType;
		uint8_t messageType;
		uint32_t time;
		uint8_t flag;
		uint8_t currArea;
		uint8_t onMission;
		uint8_t replay;
		uint32_t sequence;
		char gxt[8];
		char cutsceneName[8];
		uint16_t sourceOpcode;
		uint16_t missionId;
		int32_t timerMs;
		uint16_t objectivePhaseIndex;
		uint16_t checkpointIndex;
		uint16_t checkpointCount;
		uint8_t timerVisible;
		uint8_t timerFrozen;
		uint8_t timerDirection;
		uint8_t passFailPending;
		uint8_t playerControlState;
		uint8_t movementLocked;
		uint8_t firingLocked;
		uint8_t cameraLocked;
		uint8_t hudHidden;
		uint8_t cutscenePhase;
		uint32_t cutsceneSessionToken;
		char objective[8];
		uint8_t runtimeState;
		uint16_t objectiveVersion;
		uint16_t checkpointVersion;
		uint32_t runtimeSessionToken;
		uint32_t missionEpoch;
		uint8_t terminalReasonCode;
		uint8_t terminalSourceEventType;
		uint16_t terminalSourceOpcode;
		uint32_t terminalSourceSequence;
	};

	struct UpdateCheckpoint
	{
		int playerid;
		CVector position;
		CVector radius;
		uint16_t checkpointIndex;
		uint16_t checkpointVersion;
		uint32_t runtimeSessionToken;
		uint32_t missionEpoch;
	};

	struct RemoveCheckpoint
	{
		int playerid;
		uint16_t checkpointVersion;
		uint32_t runtimeSessionToken;
		uint32_t missionEpoch;
	};
	#pragma pack()
};

#endif
