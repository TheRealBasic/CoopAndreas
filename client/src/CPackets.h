#pragma once

#include "stdafx.h"
#include <CNetworkEntitySerializer.h>

enum eNetworkEntityType : uint8_t;
enum ePlayerDisconnectReason : uint8_t;

#include "PacketRegistry.h"


class CPackets
{
public:
	static constexpr uint8_t PLAYER_STATS_SYNCED_COUNT = 14;

	static int GetPacketSize(CPacketsID id)
	{
		static std::array<int, PACKET_ID_MAX> m_nPacketSize =
		{
			sizeof(PlayerConnected), // PLAYER_CONNECTED
			sizeof(PlayerDisconnected), // PLAYER_DISCONNECTED
			sizeof(PlayerOnFoot), // PLAYER_ONFOOT
			sizeof(PlayerBulletShot), // PLAYER_BULLET_SHOT
			sizeof(PlayerHandshake), // PLAYER_HANDSHAKE
			sizeof(PlayerPlaceWaypoint), // PLAYER_PLACE_WAYPOINT
			sizeof(VehicleSpawn), // VEHICLE_SPAWN
			sizeof(PlayerSetHost), // PLAYER_SET_HOST
			sizeof(AddExplosion), // ADD_EXPLOSION
			sizeof(VehicleRemove), // VEHICLE_REMOVE
			sizeof(VehicleIdleUpdate), // VEHICLE_IDLE_UPDATE
			sizeof(VehicleDriverUpdate), // VEHICLE_DRIVER_UPDATE
			sizeof(VehicleEnter), // VEHICLE_ENTER
			sizeof(VehicleExit), // VEHICLE_EXIT
			sizeof(VehicleDamage), // VEHICLE_DAMAGE
			sizeof(VehicleComponentAdd), // VEHICLE_COMPONENT_ADD
			sizeof(VehicleComponentRemove), // VEHICLE_COMPONENT_REMOVE
			sizeof(VehiclePassengerUpdate), // VEHICLE_PASSENGER_UPDATE
			sizeof(VehicleTrailerLinkSync), // VEHICLE_TRAILER_LINK_SYNC
			sizeof(PlayerChatMessage), // PLAYER_CHAT_MESSAGE
			sizeof(PedSpawn), // PED_SPAWN
			sizeof(PedRemove), // PED_REMOVE
			sizeof(PedOnFoot), // PED_ONFOOT
			sizeof(GameWeatherTime), // GAME_WEATHER_TIME
			0, // PED_ADD_TASK
			sizeof(PedRemoveTask), // PED_REMOVE_TASK
			sizeof(PlayerKeySync), // PLAYER_KEY_SYNC
			sizeof(PedDriverUpdate), // PED_DRIVER_UPDATE
			sizeof(PedShotSync), // PED_SHOT_SYNC
			sizeof(PedPassengerSync), // PED_PASSENGER_UPDATE
			sizeof(PlayerAimSync), // PLAYER_AIM_SYNC
			sizeof(PlayerWantedLevel), // PLAYER_WANTED_LEVEL
			sizeof(VehicleConfirm), // VEHICLE_CONFIRM
			sizeof(PedConfirm), // PED_CONFIRM
			sizeof(PlayerStats), // PLAYER_STATS
			sizeof(RebuildPlayer), // REBUILD_PLAYER
			sizeof(RespawnPlayer), // RESPAWN_PLAYER
			sizeof(AssignVehicleSyncer), // ASSIGN_VEHICLE
			sizeof(AssignPedSyncer), // ASSIGN_PED
			0, // MASS_PACKET_SEQUENCE
			sizeof(StartCutscene), // START_CUTSCENE,
			sizeof(SkipCutscene), // SKIP_CUTSCENE,
			sizeof(CutsceneSkipVoteUpdate), // CUTSCENE_SKIP_VOTE_UPDATE,
			0, // OPCODE_SYNC,
			sizeof(OnMissionFlagSync), // ON_MISSION_FLAG_SYNC,
			sizeof(MissionFlowSync), // MISSION_FLOW_SYNC,
			sizeof(UpdateEntityBlip), // UPDATE_ENTITY_BLIP,
			sizeof(RemoveEntityBlip), // REMOVE_ENTITY_BLIP,
			sizeof(AddMessageGXT), // ADD_MESSAGE_GXT,
			sizeof(RemoveMessageGXT), // REMOVE_MESSAGE_GXT,
			sizeof(ClearEntityBlips), // CLEAR_ENTITY_BLIPS,
			sizeof(PlayMissionAudio), // PLAY_MISSION_AUDIO,
			sizeof(UpdateCheckpoint), // UPDATE_CHECKPOINT,
			sizeof(RemoveCheckpoint), // REMOVE_CHECKPOINT,
			sizeof(MissionRuntimeSnapshotBegin), // MISSION_RUNTIME_SNAPSHOT_BEGIN,
			sizeof(MissionRuntimeSnapshotState), // MISSION_RUNTIME_SNAPSHOT_STATE,
			sizeof(MissionRuntimeSnapshotActor), // MISSION_RUNTIME_SNAPSHOT_ACTOR,
			sizeof(MissionRuntimeSnapshotEnd), // MISSION_RUNTIME_SNAPSHOT_END,
			0, // ENEX_SYNC,
			sizeof(CreateStaticBlip), // CREATE_STATIC_BLIP,
			sizeof(SetVehicleCreatedBy), // SET_VEHICLE_CREATED_BY
			sizeof(SetPlayerTask), // SET_PLAYER_TASK
			sizeof(PedSay), // PED_SAY
			sizeof(PedClaimOnRelease), // PED_CLAIM_ON_RELEASE
			sizeof(PedCancelClaim), // PED_CANCEL_CLAIM
			sizeof(PedResetAllClaims), // PED_RESET_ALL_CLAIMS
			sizeof(PedTakeHost), // PED_TAKE_HOST
			0, // PERFORM_TASK_SEQUENCE
			sizeof(AddProjectile), // ADD_PROJECTILE
			sizeof(FireCreate), // FIRE_CREATE
			sizeof(FireUpdate), // FIRE_UPDATE
			sizeof(FireRemove), // FIRE_REMOVE
			sizeof(TagUpdate), // TAG_UPDATE
			sizeof(UpdateAllTags), // UPDATE_ALL_TAGS
			sizeof(GangZoneState), // GANG_ZONE_STATE
			sizeof(GangGroupMembershipUpdate), // GANG_GROUP_MEMBERSHIP_UPDATE
			sizeof(GangRelationshipUpdate), // GANG_RELATIONSHIP_UPDATE
			sizeof(GangWarLifecycleEvent), // GANG_WAR_LIFECYCLE_EVENT
			sizeof(TeleportPlayerScripted), // TELEPORT_PLAYER_SCRIPTED
			sizeof(PickupSnapshotBegin), // PICKUP_SNAPSHOT_BEGIN
			sizeof(PickupSnapshotEntry), // PICKUP_SNAPSHOT_ENTRY
			sizeof(PickupSnapshotEnd), // PICKUP_SNAPSHOT_END
			sizeof(PickupSnapshotRequest), // PICKUP_SNAPSHOT_REQUEST
			sizeof(PickupCollectRequest), // PICKUP_COLLECT_REQUEST
			sizeof(PickupStateDelta), // PICKUP_STATE_DELTA
			sizeof(PickupDropCreate), // PICKUP_DROP_CREATE
			sizeof(PickupDropResolve), // PICKUP_DROP_RESOLVE
			sizeof(PropertyStateSnapshotBegin), // PROPERTY_STATE_SNAPSHOT_BEGIN
			sizeof(PropertyStateSnapshotEntry), // PROPERTY_STATE_SNAPSHOT_ENTRY
			sizeof(PropertyStateSnapshotEnd), // PROPERTY_STATE_SNAPSHOT_END
			sizeof(PropertyStateDelta), // PROPERTY_STATE_DELTA
			sizeof(SubmissionMissionSnapshotBegin), // SUBMISSION_MISSION_SNAPSHOT_BEGIN
			sizeof(SubmissionMissionSnapshotEntry), // SUBMISSION_MISSION_SNAPSHOT_ENTRY
			sizeof(SubmissionMissionSnapshotEnd), // SUBMISSION_MISSION_SNAPSHOT_END
			sizeof(SubmissionMissionStateDelta), // SUBMISSION_MISSION_STATE_DELTA
			sizeof(PlayerJetpackTransition), // PLAYER_JETPACK_TRANSITION
			sizeof(CheatEffectTrigger), // CHEAT_EFFECT_TRIGGER
			sizeof(PlayerSniperAimMarkerState), // PLAYER_SNIPER_AIM_MARKER_STATE
		};

		return m_nPacketSize[id];
	}

	struct PlayerConnected
	{
		int id;
		bool isAlreadyConnected; // prevents spam in the chat when connecting by distinguishing already connected players from newly joined ones
		char name[32 + 1];
		uint32_t version;
	};

	struct PlayerDisconnected
	{
		int id;
		ePlayerDisconnectReason reason;
		uint32_t version;
	};

	struct PlayerOnFoot
	{
		int id = 0;
		CVector position = CVector();
		CVector velocity = CVector();
		float currentRotation = 0.0f;
		float aimingRotation = 0.0f;
		unsigned char health = 100;
		unsigned char armour = 0;
		unsigned char weapon = 0;
		unsigned char weaponState = 0;
		unsigned short ammo = 0;
		bool ducking = false;
		bool hasJetpack = false;
		char fightingStyle = 4;
	};

	enum eJetpackTransitionIntent : uint8_t
	{
		JETPACK_TRANSITION_ACQUIRE = 0,
		JETPACK_TRANSITION_REMOVE = 1,
		JETPACK_TRANSITION_FORCED_REMOVE = 2
	};

	struct PlayerJetpackTransition
	{
		int playerid = 0;
		uint8_t intent = JETPACK_TRANSITION_ACQUIRE;
		bool hasJetpack = false;
	};

	enum eCheatEffectType : uint8_t
	{
		CHEAT_EFFECT_WORLD_WEATHER_TIME = 0,
		CHEAT_EFFECT_PLAYER_WANTED_LEVEL = 1,
		CHEAT_EFFECT_PLAYER_STATS = 2
	};

	struct CheatEffectTrigger
	{
		uint8_t effectType = CHEAT_EFFECT_WORLD_WEATHER_TIME;
		unsigned char newWeather = 0;
		unsigned char oldWeather = 0;
		unsigned char forcedWeather = 0;
		unsigned char currentMonth = 0;
		unsigned char currentDay = 0;
		unsigned char currentHour = 0;
		unsigned char currentMinute = 0;
		unsigned int gameTickCount = 0;
		uint8_t wantedLevel = 0;
		float stats[PLAYER_STATS_SYNCED_COUNT]{};
		int money = 0;
	};

	#pragma pack(1)
	struct PlayerBulletShot
	{
		int playerid;
		int targetid;
		CVector startPos;
		CVector endPos;
		CColPoint colPoint;
		int incrementalHit;
		unsigned char entityType;
		unsigned char shotWeaponId;
		bool moonSniperActive;
		float shotSizeMultiplier;
	};

	struct PlayerSniperAimMarkerState
	{
		int playerid;
		CVector source;
		CVector direction;
		float range;
		bool visible;
		uint32_t tick;
	};

	struct PlayerHandshake
	{
		uint8_t stage;
		uint16_t protocolVersion;
		uint64_t capabilityBitmap;
		uint32_t nonce;
		uint32_t responseHash;
		int yourid;
	};

	struct PlayerPlaceWaypoint
	{
		int playerid;
		bool place;
		CVector position;
		uint8_t currArea;
	};

	struct PlayerSetHost
	{
		int playerid;
		uint32_t missionEpoch;
		uint32_t sequenceId;
	};

	struct AddExplosion
	{
		unsigned char type;
		CVector pos;
		int time;
		bool usesSound;
		float cameraShake;
		bool isVisible;
	};

	struct VehicleSpawn
	{
		int vehicleid;
		unsigned char tempid;
		unsigned short modelid;
		CVector pos;
		float rot;
		unsigned char color1;
		unsigned char color2;
		unsigned char createdBy;
	};

	struct VehicleRemove
	{
		int vehicleid;
	};

	struct VehicleIdleUpdate
	{
		int vehicleid;
		CVector pos;
		CVector rot;
		CVector roll;
		CVector velocity;
		CVector turnSpeed;
		unsigned char color1;
		unsigned char color2;
		float health;
		char paintjob;
		float planeGearState;
		uint8_t hydraulicsControlState;
		uint8_t hydraulicsTransitionMask;
		uint16_t hydraulicsTransitionSequence;
		unsigned char locked;
	};

	struct VehicleDriverUpdate
	{
		int playerid;
		int vehicleid;
		CVector pos;
		CVector rot;
		CVector roll;
		CVector velocity;
		unsigned char playerHealth;
		unsigned char playerArmour;
		unsigned char weapon;
		unsigned short ammo;
		unsigned char color1;
		unsigned char color2;
		float health;
		char paintjob;
		float bikeLean;
		unsigned short miscComponentAngle; // hydra thrusters
		float planeGearState;
		uint8_t hydraulicsControlState;
		uint8_t hydraulicsTransitionMask;
		uint16_t hydraulicsTransitionSequence;
		unsigned char locked;
	};

	struct VehicleEnter
	{
		int playerid;
		int vehicleid;
		unsigned char seatid : 3;
		unsigned char force : 1;
		unsigned char passenger : 1;
	};

	struct VehicleExit
	{
		int playerid;
		bool force;
	};

	struct VehicleDamage
	{
		int vehicleid;
		CDamageManager damageManager;
	};

	struct VehicleComponentAdd
	{
		int vehicleid;
		int componentid;
	};

	struct VehicleComponentRemove
	{
		int vehicleid;
		int componentid;
	};

	struct VehiclePassengerUpdate
	{
		int playerid;
		int vehicleid;
		unsigned char playerHealth;
		unsigned char playerArmour;
		unsigned char weapon;
		unsigned short ammo;
		unsigned char driveby;
		unsigned char seatid;
		unsigned char gamepadFlags;
		signed char radioStation;
		unsigned char radioState;
	};

	struct VehicleTrailerLinkSync
	{
		enum eDetachReason : uint8_t
		{
			DETACH_REASON_NONE = 0,
			DETACH_REASON_MANUAL = 1,
			DETACH_REASON_FORCE = 2,
			DETACH_REASON_ENTITY_REMOVED = 3
		};

		int tractorVehicleId;
		int trailerVehicleId;
		uint8_t attachState;
		uint8_t detachReason;
		uint64_t timestampMs;
		uint32_t linkVersion;
	};

	struct PlayerChatMessage
	{
		int playerid;
		wchar_t message[CChat::MAX_MESSAGE_SIZE+1];
	};

	struct PedSpawn
	{
		int pedid;
		unsigned char tempid;
		short modelId;
		unsigned char pedType;
		CVector pos;
		unsigned char createdBy;
		char specialModelName[8];
	};

	struct PedRemove
	{
		int pedid;
	};

	struct PedOnFoot
	{
		int pedid = 0;
		CVector pos = CVector();
		CVector velocity = CVector();
		unsigned char health = 100;
		unsigned char armour = 0;
		unsigned char weapon = 0;
		unsigned short ammo = 0;
		float aimingRotation = 0.0f;
		float currentRotation = 0.0f;
		int lookDirection = 0;
		struct
		{
			unsigned char moveState : 3;
			unsigned char ducked : 1;
			unsigned char aiming : 1;
		};
		unsigned char fightingStyle = 4;
		CVector weaponAim;
	};
	
	struct GameWeatherTime
	{
		unsigned char newWeather;
		unsigned char oldWeather;
		unsigned char forcedWeather;
		unsigned char currentMonth;
		unsigned char currentDay;
		unsigned char currentHour;
		unsigned char currentMinute;
		unsigned int gameTickCount;
	};

	struct PedRemoveTask
	{
		int pedid;
		eTaskType taskid;
	};

	struct PlayerKeySync
	{
		int playerid;
		CCompressedControllerState newState;
		uint8_t currArea;
	};

	struct PedDriverUpdate
	{
		int pedid;
		int vehicleid;
		CVector pos;
		CVector rot;
		CVector roll;
		CVector velocity;
		CVector turnSpeed;
		unsigned char pedHealth;
		unsigned char pedArmour;
		unsigned char weapon;
		unsigned short ammo;
		unsigned char color1;
		unsigned char color2;
		float health;
		char paintjob;
		float bikeLean;
		union
		{
			float controlPedaling;
			float planeGearState;
		};
		unsigned char locked;
		float gasPedal;
		float breakPedal;
		uint8_t drivingStyle;
		uint8_t carMission;
		int8_t cruiseSpeed;
		uint8_t ctrlFlags;
		uint8_t movementFlags;
		int targetVehicleId;
		CVector destinationCoors;
	};

	struct PedShotSync
	{
		int pedid;
		CVector origin;
		CVector effect;
		CVector target;
	};

	struct PedPassengerSync
	{
		int pedid;
		int vehicleid;
		unsigned char health;
		unsigned char armour;
		unsigned char weapon;
		unsigned short ammo;
		unsigned char seatid;
	};

	struct PlayerAimSync
	{
		int playerid;
		unsigned char cameraMode;
		unsigned char weaponCameraMode;
		float cameraFov;
		CVector front;
		CVector	source;
		CVector	up;
		float lookPitch;
		float orientation;
	};

	struct PlayerWantedLevel
	{
		int playerid;
		uint8_t wantedLevel;
	};

	struct VehicleConfirm
	{
		unsigned char tempid = 255;
		int vehicleid;
	};

	struct PedConfirm
	{
		unsigned char tempid = 255;
		int pedid;
	};

	struct PlayerStats
	{
		int playerid;
		float stats[PLAYER_STATS_SYNCED_COUNT];
		int money;
	};
	static_assert(sizeof(PlayerStats) == 64, "CPackets::PlayerStats layout mismatch");

	struct RebuildPlayer
	{
		int playerid;
		CPedClothesDesc clothesData;
	};

	struct AssignVehicleSyncer
	{
		int vehicleid;
	};

	struct AssignPedSyncer
	{
		int pedid;
	};

	struct RespawnPlayer
	{
		int playerid;
	};

	struct StartCutscene
	{
		char name[8];
		uint8_t currArea; // AKA interior
		uint32_t sessionToken;
	};

	struct SkipCutscene
	{
		int playerid;
		int votes;
		uint32_t sessionToken;
	};

	struct CutsceneSkipVoteUpdate
	{
		int voterid;
		int currentVotes;
		int requiredVotes;
		uint32_t sessionToken;
		bool thresholdReached;
	};

	struct OnMissionFlagSync 
	{
		uint8_t bOnMission : 1;
		uint32_t missionEpoch = 0;
		uint32_t sequenceId = 0;
	};

	enum eMissionFlowEventType : uint8_t
	{
		MISSION_FLOW_EVENT_NONE = 0,
		MISSION_FLOW_EVENT_CUTSCENE_INTRO = 1,
		MISSION_FLOW_EVENT_OBJECTIVE = 2,
		MISSION_FLOW_EVENT_FAIL = 3,
		MISSION_FLOW_EVENT_PASS = 4,
		MISSION_FLOW_EVENT_STATE_UPDATE = 5,
		MISSION_FLOW_EVENT_CUTSCENE_SKIP = 6,
		MISSION_FLOW_EVENT_CUTSCENE_END = 7,
		MISSION_FLOW_EVENT_PARTICIPANT_DEATH = 8,
		MISSION_FLOW_EVENT_PARTICIPANT_INCAPACITATED = 9,
		MISSION_FLOW_EVENT_RESPAWN_ELIGIBILITY = 10,
		MISSION_FLOW_EVENT_PARTICIPANT_RESPAWNED = 11,
		MISSION_FLOW_EVENT_TARGET_STATE = 12
	};

	enum eMissionTerminalReasonCode : uint8_t
	{
		MISSION_TERMINAL_REASON_NONE = 0,
		MISSION_TERMINAL_REASON_PASS = 1,
		MISSION_TERMINAL_REASON_FAIL = 2,
		MISSION_TERMINAL_REASON_ON_MISSION_CLEARED = 3
	};

	struct MissionFlowSync
	{
		// Shared side-content payload used by mission/sync modules.
		// Applies to school attempts, race-style content (launch/start + checkpoint progression),
		// timer-driven side activities, and deterministic pass/fail replay.
		// Server replays the latest payload with replay=1 for reconnect + late-join hydration.
		uint8_t eventType = MISSION_FLOW_EVENT_NONE;
		uint8_t messageType = 0; // mirrors AddMessageGXT::type
		uint32_t time = 0;
		uint8_t flag = 0;
		uint8_t currArea = 0;
		uint8_t onMission = 0;
		uint8_t replay = 0; // server-generated replay for reconnecting peers
		uint32_t sequence = 0;
		char gxt[8]{};
		char cutsceneName[8]{};
		uint16_t sourceOpcode = 0;
		uint16_t missionId = 0;           // Launch/start identity (Mission.LoadAndLaunchInternal / race start script launch)
		int32_t timerMs = 0;              // Canonical timer value in milliseconds
		uint16_t objectivePhaseIndex = 0; // Monotonic objective phase index
		uint16_t checkpointIndex = 0;     // Monotonic progression index (checkpoint + objective updates)
		uint16_t checkpointCount = 0;     // Number of checkpoint handles created for this attempt
		uint8_t timerVisible = 0;         // 1 when on-screen timer should be visible
		uint8_t timerFrozen = 0;          // 1 when on-screen timer is frozen
		uint8_t timerDirection = 0;       // TimerDirection mirrored from display timer opcode
		uint8_t passFailPending = 0;      // 0 = none, 1 = pass, 2 = fail (terminal outcome latch)
		uint8_t playerControlState = 0;   // set_player_control mirror for scripted transitions
		uint8_t movementLocked = 0;       // 1 = movement/gameplay locomotion lock is active
		uint8_t aimingLocked = 0;         // 1 = scripted aiming lock is active
		uint8_t firingLocked = 0;         // 1 = scripted firing lock is active
		uint8_t cameraLocked = 0;         // 1 = scripted camera control lock is active
		uint8_t hudHidden = 0;            // 1 = scripted HUD hide lock is active
		uint8_t cutscenePhase = 0;        // 0 = none, 1 = intro, 2 = skip, 3 = end
		uint32_t cutsceneSessionToken = 0;
		uint32_t objectiveTextToken = 0;  // Stable token for objective text key (0 = cleared)
		uint8_t objectiveTextSemantics = 0; // 0 = none, 1 = replace, 2 = clear
		char objective[8]{};
		uint8_t runtimeState = 0;
		uint16_t objectiveVersion = 0;
		uint16_t checkpointVersion = 0;
		uint32_t runtimeSessionToken = 0;
		uint32_t missionEpoch = 0;
		uint32_t sequenceId = 0;
		uint8_t terminalReasonCode = MISSION_TERMINAL_REASON_NONE;
		uint8_t terminalSourceEventType = MISSION_FLOW_EVENT_NONE;
		uint16_t terminalSourceOpcode = 0;
		uint32_t terminalSourceSequence = 0;
		uint8_t respawnEligible = 0;
		uint8_t participantDeathCount = 0;
		uint8_t participantIncapacitationCount = 0;
		uint8_t respawnCount = 0;
		uint8_t missionFailThreshold = 1;
		uint8_t incapacitationFailThreshold = UINT8_MAX;
		uint8_t vehicleTaskState = 0;
		uint8_t pursuitState = 0;
		uint8_t destroyEscapeState = 0;
		uint16_t vehicleTaskSequence = 0;
		uint8_t targetObjectiveType = 0;
		uint8_t targetLifecycleState = 0;
		int32_t targetEntityNetworkId = -1;
		uint8_t targetEntityType = 0;
		uint16_t targetStateSequence = 0;
		uint8_t terminalTieBreaker = 0;
	};

	struct UpdateEntityBlip
	{
		int playerid;
		eNetworkEntityType entityType;
		int entityId;
		bool isFriendly;
		uint8_t color;
		uint8_t display;
		uint8_t scale;
	};

	struct RemoveEntityBlip
	{
		int playerid;
		eNetworkEntityType entityType;
		int entityId;
	};

	struct AddMessageGXT
	{
		int playerid;
		// 0 - COMMAND_PRINT
		// 1 - COMMAND_PRINT_BIG
		// 2 - COMMAND_PRINT_NOW
		// 3 - COMMAND_PRINT_HELP
		uint8_t type; 
		uint32_t time;
		uint8_t flag;
		char gxt[8];
	};

	struct RemoveMessageGXT
	{
		int playerid;
		char gxt[8];
	};

	struct ClearEntityBlips
	{
		int playerid;
	};

	struct PlayMissionAudio
	{
		uint8_t slotid;
		int audioid;
	};

	struct UpdateCheckpoint
	{
		int playerid;
		CVector position;
		CVector radius;
		uint16_t checkpointIndex = 0;
		uint16_t checkpointVersion = 0;
		uint32_t runtimeSessionToken = 0;
		uint32_t missionEpoch = 0;
		uint32_t sequenceId = 0;
	};

	struct RemoveCheckpoint
	{
		int playerid;
		uint16_t checkpointVersion = 0;
		uint32_t runtimeSessionToken = 0;
		uint32_t missionEpoch = 0;
		uint32_t sequenceId = 0;
	};

	struct MissionRuntimeSnapshotBegin
	{
		uint32_t snapshotVersion = 0;
		uint8_t actorCount = 0;
	};

	struct MissionRuntimeSnapshotState
	{
		uint8_t runtimeState = 0;
		uint8_t onMission = 0;
		uint16_t missionId = 0;
		uint16_t objectivePhaseIndex = 0;
		uint32_t objectiveTextToken = 0;
		uint8_t objectiveTextSemantics = 0;
		char objective[8]{};
		int32_t timerMs = 0;
		uint8_t timerVisible = 0;
		uint8_t timerFrozen = 0;
		uint8_t timerDirection = 0;
		uint16_t checkpointIndex = 0;
		uint16_t checkpointCount = 0;
		uint8_t playerControlState = 0;
		uint8_t movementLocked = 0;
		uint8_t aimingLocked = 0;
		uint8_t firingLocked = 0;
		uint8_t cameraLocked = 0;
		uint8_t hudHidden = 0;
		uint16_t objectiveVersion = 0;
		uint16_t checkpointVersion = 0;
		uint32_t runtimeSessionToken = 0;
		uint32_t missionEpoch = 0;
		uint32_t sequenceId = 0;
		uint8_t terminalReasonCode = MISSION_TERMINAL_REASON_NONE;
		uint8_t terminalSourceEventType = MISSION_FLOW_EVENT_NONE;
		uint16_t terminalSourceOpcode = 0;
		uint32_t terminalSourceSequence = 0;
		uint8_t passFailPending = 0;
		uint8_t cutscenePhase = 0;
		uint32_t cutsceneSessionToken = 0;
		uint8_t respawnEligible = 0;
		uint8_t participantDeathCount = 0;
		uint8_t participantIncapacitationCount = 0;
		uint8_t respawnCount = 0;
		uint8_t missionFailThreshold = 1;
		uint8_t incapacitationFailThreshold = UINT8_MAX;
		uint8_t vehicleTaskState = 0;
		uint8_t pursuitState = 0;
		uint8_t destroyEscapeState = 0;
		uint16_t vehicleTaskSequence = 0;
		uint8_t targetObjectiveType = 0;
		uint8_t targetLifecycleState = 0;
		int32_t targetEntityNetworkId = -1;
		uint8_t targetEntityType = 0;
		uint16_t targetStateSequence = 0;
		uint8_t terminalTieBreaker = 0;
	};

	struct MissionRuntimeSnapshotActor
	{
		uint8_t actorType = 0;
		int32_t actorNetworkId = 0;
		uint8_t roleFlags = 0;
		uint8_t isAlive = 0;
		uint32_t missionEpoch = 0;
		uint32_t scriptLocalIdentifier = 0;
		uint16_t sourceOpcode = 0;
		uint8_t sourceSlot = 0;
	};

	struct MissionRuntimeSnapshotEnd
	{
		uint32_t snapshotVersion = 0;
	};

	struct CreateStaticBlip
	{
		CVector position;
		int8_t sprite;
		uint8_t display : 2;
		uint8_t type : 1; // 0 - BLIP_CONTACT_POINT, 1 - BLIP_COORD
		uint8_t trackingBlip : 1;
		uint8_t shortRange : 1;
		uint8_t friendly : 1; // It is affected by BLIP_COLOUR_THREAT.   
		uint8_t coordBlipAppearance : 2; // see eBlipAppearance
		uint8_t size : 3;
		uint8_t color : 4;
	};

	struct SetVehicleCreatedBy
	{
		int vehicleid;
		uint8_t createdBy;
	};

	struct SetPlayerTask
	{
		int playerid;
		int taskType;
		CVector position;
		float rotation;
		bool toggle;
	};

	struct PedSay
	{
		int entityid : 31;
		int isPlayer : 1;
		int16_t phraseId;
		int startTimeDelay;
		uint8_t overrideSilence : 1;
		uint8_t isForceAudible : 1;
		uint8_t isFrontEnd : 1;
	};

	struct PedClaimOnRelease
	{
		int pedid;
	};

	struct PedCancelClaim
	{
		int pedid;
	};

	struct PedResetAllClaims
	{
		int pedid;
	};

	struct PedTakeHost
	{
		int pedid;
		bool allowReturnToPreviousHost;
	};

	struct AddProjectile
	{
		CNetworkEntitySerializer creator;
		uint8_t projectileType; // eWeaponType
		CVector origin;
		float force;
		CVector dir;
		CNetworkEntitySerializer target;
	};


	struct FireCreate
	{
		uint32_t fireId;
		CVector position;
		float radius;
		uint8_t fireType;
		int ownerPlayerId;
		uint8_t sourceType;
		uint32_t timestampMs;
	};

	struct FireUpdate
	{
		uint32_t fireId;
		CVector position;
		float radius;
		uint8_t fireType;
		int ownerPlayerId;
		uint8_t sourceType;
		uint32_t timestampMs;
	};

	struct FireRemove
	{
		uint32_t fireId;
		uint32_t timestampMs;
	};

	struct TagUpdate
	{
		int16_t pos_x;
		int16_t pos_y;
		int16_t pos_z;
		uint8_t alpha;
	};

	struct UpdateAllTags
	{
		TagUpdate tags[150];
	};

	struct GangZoneState
	{
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t state;
		uint8_t currArea;
	};

	struct GangGroupMembershipUpdate
	{
		uint32_t sequence;
		int pedNetworkId;
		uint8_t gangGroupId;
		uint8_t action; // 0 add, 1 remove, 2 clear group
	};

	struct GangRelationshipUpdate
	{
		uint32_t sequence;
		uint8_t sourceGangGroupId;
		uint8_t targetGangGroupId;
		uint8_t relationshipFlags; // bit0 friendly, bit1 hostile
	};

	struct GangWarLifecycleEvent
	{
		uint32_t sequence;
		uint8_t eventType; // 1 start trigger, 2 wave progression, 3 outcome, 4 territory ownership update
		uint8_t warState;
		uint8_t warPhase;
		uint8_t outcome; // 0 unknown/in-progress, 1 win, 2 loss
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t zoneState;
		uint8_t currArea;
	};

	struct TeleportPlayerScripted
	{
		int playerid;
		CVector pos;
		float heading;
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
		uint8_t action; // 0 upsert, 1 remove/reset
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

	struct ReplicatedCheckpointState
	{
		uint16_t currentIndex;
		uint16_t totalCount;
		uint8_t activeFlag;
	};

	struct ReplicatedTimerState
	{
		uint32_t startTick;
		int32_t remaining;
		uint8_t paused;
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
		uint8_t outcome; // 0 none, 1 pass, 2 fail
		uint8_t participantCount;
		uint8_t currArea;
		ReplicatedCheckpointState checkpointState{};
		ReplicatedTimerState timerState{};
		uint64_t stateTimestampMs;
		uint32_t stateVersion;
	};

	struct SubmissionMissionSnapshotEnd
	{
		uint32_t snapshotVersion;
	};

	struct SubmissionMissionStateDelta : public SubmissionMissionSnapshotEntry
	{
		uint8_t action; // 0 upsert, 1 clear
	};
};
