#pragma once

#ifndef _CPACKET_H_
	#define _CPACKET_H_

#include "CVector.h"
#include <cstdint>

enum CPacketsID : unsigned short
{
	PLAYER_CONNECTED,
	PLAYER_DISCONNECTED,
	PLAYER_ONFOOT,
	PLAYER_BULLET_SHOT,
	PLAYER_HANDSHAKE,
	PLAYER_PLACE_WAYPOINT,
	VEHICLE_SPAWN,
	PLAYER_SET_HOST,
	ADD_EXPLOSION,
	VEHICLE_REMOVE,
	VEHICLE_IDLE_UPDATE,
	VEHICLE_DRIVER_UPDATE,
	VEHICLE_ENTER,
	VEHICLE_EXIT,
	VEHICLE_DAMAGE,
	VEHICLE_COMPONENT_ADD,
	VEHICLE_COMPONENT_REMOVE,
	VEHICLE_PASSENGER_UPDATE,
	VEHICLE_TRAILER_LINK_SYNC,
	PLAYER_CHAT_MESSAGE,
	PED_SPAWN,
	PED_REMOVE,
	PED_ONFOOT,
	GAME_WEATHER_TIME,
	PED_ADD_TASK,
	PED_REMOVE_TASK,
	PLAYER_KEY_SYNC,
	PED_DRIVER_UPDATE,
	PED_SHOT_SYNC,
	PED_PASSENGER_UPDATE,
	PLAYER_AIM_SYNC,
	PLAYER_WANTED_LEVEL,
	VEHICLE_CONFIRM,
	PED_CONFIRM,
	PLAYER_STATS,
	REBUILD_PLAYER,
	RESPAWN_PLAYER,
	ASSIGN_VEHICLE,
	ASSIGN_PED,
	MASS_PACKET_SEQUENCE,
	START_CUTSCENE,
	SKIP_CUTSCENE,
	CUTSCENE_SKIP_VOTE_UPDATE,
	OPCODE_SYNC,
	ON_MISSION_FLAG_SYNC,
	MISSION_FLOW_SYNC,
	UPDATE_ENTITY_BLIP,
	REMOVE_ENTITY_BLIP,
	ADD_MESSAGE_GXT,
	REMOVE_MESSAGE_GXT,
	CLEAR_ENTITY_BLIPS,
	PLAY_MISSION_AUDIO,
	UPDATE_CHECKPOINT,
	REMOVE_CHECKPOINT,
	ENEX_SYNC,
	CREATE_STATIC_BLIP,
	SET_VEHICLE_CREATED_BY,
	SET_PLAYER_TASK,
	PED_SAY,
	PED_CLAIM_ON_RELEASE,
	PED_CANCEL_CLAIM,
	PED_RESET_ALL_CLAIMS,
	PED_TAKE_HOST,
	PERFORM_TASK_SEQUENCE,
	ADD_PROJECTILE,
	FIRE_CREATE,
	FIRE_UPDATE,
	FIRE_REMOVE,
	TAG_UPDATE,
	UPDATE_ALL_TAGS,
	GANG_ZONE_STATE,
	GANG_GROUP_MEMBERSHIP_UPDATE,
	GANG_RELATIONSHIP_UPDATE,
	GANG_WAR_LIFECYCLE_EVENT,
	TELEPORT_PLAYER_SCRIPTED,
	PICKUP_SNAPSHOT_BEGIN,
	PICKUP_SNAPSHOT_ENTRY,
	PICKUP_SNAPSHOT_END,
	PICKUP_SNAPSHOT_REQUEST,
	PICKUP_COLLECT_REQUEST,
	PICKUP_STATE_DELTA,
	PICKUP_DROP_CREATE,
	PICKUP_DROP_RESOLVE,
	PROPERTY_STATE_SNAPSHOT_BEGIN,
	PROPERTY_STATE_SNAPSHOT_ENTRY,
	PROPERTY_STATE_SNAPSHOT_END,
	PROPERTY_STATE_DELTA,
	SUBMISSION_MISSION_SNAPSHOT_BEGIN,
	SUBMISSION_MISSION_SNAPSHOT_ENTRY,
	SUBMISSION_MISSION_SNAPSHOT_END,
	SUBMISSION_MISSION_STATE_DELTA,
	PLAYER_JETPACK_TRANSITION,
	CHEAT_EFFECT_TRIGGER,
	PACKET_ID_MAX
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
		uint8_t stage;
		uint16_t progress;
		int32_t timerMs;
		int32_t rewardCash;
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

#endif
