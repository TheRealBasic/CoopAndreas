#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>

// Shared packet id source of truth for client and server.
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
	MISSION_RUNTIME_SNAPSHOT_BEGIN,
	MISSION_RUNTIME_SNAPSHOT_STATE,
	MISSION_RUNTIME_SNAPSHOT_ACTOR,
	MISSION_RUNTIME_SNAPSHOT_END,
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
	PLAYER_SNIPER_AIM_MARKER_STATE,
	PACKET_ID_MAX
};

namespace PacketRegistry
{
	enum class PacketDirection : uint8_t
	{
		None = 0,
		ClientToServer = 1,
		ServerToClient = 2,
		Bidirectional = 3
	};

	enum class PacketReliability : uint8_t
	{
		Unreliable,
		Reliable,
		Mixed
	};

	enum class ReceiverRole : uint8_t
	{
		Client,
		Server
	};

	enum CapabilityFlags : uint64_t
	{
		CAP_BASELINE = 1ull << 0,
		CAP_PACKET_REGISTRY_V1 = 1ull << 1,
		CAP_PICKUP_FAMILY = 1ull << 2,
		CAP_PROPERTY_FAMILY = 1ull << 3,
		CAP_SUBMISSION_FAMILY = 1ull << 4
	};

	constexpr uint16_t PROTOCOL_VERSION = 2;
	constexpr uint64_t PROTOCOL_CAPABILITIES = CAP_BASELINE | CAP_PACKET_REGISTRY_V1 | CAP_PICKUP_FAMILY | CAP_PROPERTY_FAMILY | CAP_SUBMISSION_FAMILY;

	struct PacketContract
	{
		CPacketsID id;
		const char* name;
		PacketDirection direction;
		PacketReliability reliability;
		uint16_t minPayloadSize;
	};

	struct ListenerValidationReport
	{
		size_t requiredCount = 0;
		size_t registeredCount = 0;
		size_t missingCount = 0;
		size_t unexpectedCount = 0;
		std::string details;
		bool ok = true;
	};

	inline std::array<PacketContract, PACKET_ID_MAX> BuildContracts()
	{
		std::array<PacketContract, PACKET_ID_MAX> contracts{};
		for (unsigned short i = 0; i < PACKET_ID_MAX; ++i)
		{
			contracts[i] = { static_cast<CPacketsID>(i), "UNSPECIFIED", PacketDirection::None, PacketReliability::Mixed, 0 };
		}

		auto set = [&contracts](CPacketsID id, const char* name, PacketDirection direction, PacketReliability reliability, uint16_t minPayload)
		{
			contracts[id] = { id, name, direction, reliability, minPayload };
		};

		// bidirectional
		set(PLAYER_CONNECTED, "PLAYER_CONNECTED", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAYER_ONFOOT, "PLAYER_ONFOOT", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PLAYER_BULLET_SHOT, "PLAYER_BULLET_SHOT", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PLAYER_PLACE_WAYPOINT, "PLAYER_PLACE_WAYPOINT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(ADD_EXPLOSION, "ADD_EXPLOSION", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_SPAWN, "VEHICLE_SPAWN", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_REMOVE, "VEHICLE_REMOVE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_ENTER, "VEHICLE_ENTER", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_EXIT, "VEHICLE_EXIT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_DRIVER_UPDATE, "VEHICLE_DRIVER_UPDATE", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(VEHICLE_IDLE_UPDATE, "VEHICLE_IDLE_UPDATE", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(VEHICLE_DAMAGE, "VEHICLE_DAMAGE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_COMPONENT_ADD, "VEHICLE_COMPONENT_ADD", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_COMPONENT_REMOVE, "VEHICLE_COMPONENT_REMOVE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(VEHICLE_PASSENGER_UPDATE, "VEHICLE_PASSENGER_UPDATE", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(VEHICLE_TRAILER_LINK_SYNC, "VEHICLE_TRAILER_LINK_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAYER_CHAT_MESSAGE, "PLAYER_CHAT_MESSAGE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_SPAWN, "PED_SPAWN", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_REMOVE, "PED_REMOVE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_ONFOOT, "PED_ONFOOT", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(GAME_WEATHER_TIME, "GAME_WEATHER_TIME", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAYER_KEY_SYNC, "PLAYER_KEY_SYNC", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PED_ADD_TASK, "PED_ADD_TASK", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_DRIVER_UPDATE, "PED_DRIVER_UPDATE", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PED_SHOT_SYNC, "PED_SHOT_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_PASSENGER_UPDATE, "PED_PASSENGER_UPDATE", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PLAYER_AIM_SYNC, "PLAYER_AIM_SYNC", PacketDirection::Bidirectional, PacketReliability::Unreliable, 0);
		set(PLAYER_WANTED_LEVEL, "PLAYER_WANTED_LEVEL", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAYER_STATS, "PLAYER_STATS", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(REBUILD_PLAYER, "REBUILD_PLAYER", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(RESPAWN_PLAYER, "RESPAWN_PLAYER", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(START_CUTSCENE, "START_CUTSCENE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(SKIP_CUTSCENE, "SKIP_CUTSCENE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(OPCODE_SYNC, "OPCODE_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(ON_MISSION_FLAG_SYNC, "ON_MISSION_FLAG_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(MISSION_FLOW_SYNC, "MISSION_FLOW_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(UPDATE_ENTITY_BLIP, "UPDATE_ENTITY_BLIP", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(REMOVE_ENTITY_BLIP, "REMOVE_ENTITY_BLIP", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(ADD_MESSAGE_GXT, "ADD_MESSAGE_GXT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(REMOVE_MESSAGE_GXT, "REMOVE_MESSAGE_GXT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(CLEAR_ENTITY_BLIPS, "CLEAR_ENTITY_BLIPS", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAY_MISSION_AUDIO, "PLAY_MISSION_AUDIO", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(UPDATE_CHECKPOINT, "UPDATE_CHECKPOINT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(REMOVE_CHECKPOINT, "REMOVE_CHECKPOINT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(ENEX_SYNC, "ENEX_SYNC", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(CREATE_STATIC_BLIP, "CREATE_STATIC_BLIP", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(SET_VEHICLE_CREATED_BY, "SET_VEHICLE_CREATED_BY", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(SET_PLAYER_TASK, "SET_PLAYER_TASK", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_SAY, "PED_SAY", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PED_RESET_ALL_CLAIMS, "PED_RESET_ALL_CLAIMS", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PERFORM_TASK_SEQUENCE, "PERFORM_TASK_SEQUENCE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(ADD_PROJECTILE, "ADD_PROJECTILE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(TAG_UPDATE, "TAG_UPDATE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(UPDATE_ALL_TAGS, "UPDATE_ALL_TAGS", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(GANG_ZONE_STATE, "GANG_ZONE_STATE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(GANG_GROUP_MEMBERSHIP_UPDATE, "GANG_GROUP_MEMBERSHIP_UPDATE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(GANG_RELATIONSHIP_UPDATE, "GANG_RELATIONSHIP_UPDATE", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(GANG_WAR_LIFECYCLE_EVENT, "GANG_WAR_LIFECYCLE_EVENT", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(TELEPORT_PLAYER_SCRIPTED, "TELEPORT_PLAYER_SCRIPTED", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PROPERTY_STATE_DELTA, "PROPERTY_STATE_DELTA", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(SUBMISSION_MISSION_STATE_DELTA, "SUBMISSION_MISSION_STATE_DELTA", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);
		set(PLAYER_JETPACK_TRANSITION, "PLAYER_JETPACK_TRANSITION", PacketDirection::Bidirectional, PacketReliability::Reliable, 0);

		// server -> client
		set(PLAYER_DISCONNECTED, "PLAYER_DISCONNECTED", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(MISSION_RUNTIME_SNAPSHOT_BEGIN, "MISSION_RUNTIME_SNAPSHOT_BEGIN", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(MISSION_RUNTIME_SNAPSHOT_STATE, "MISSION_RUNTIME_SNAPSHOT_STATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(MISSION_RUNTIME_SNAPSHOT_ACTOR, "MISSION_RUNTIME_SNAPSHOT_ACTOR", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(MISSION_RUNTIME_SNAPSHOT_END, "MISSION_RUNTIME_SNAPSHOT_END", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PLAYER_HANDSHAKE, "PLAYER_HANDSHAKE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PLAYER_SET_HOST, "PLAYER_SET_HOST", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(VEHICLE_CONFIRM, "VEHICLE_CONFIRM", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PED_CONFIRM, "PED_CONFIRM", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(ASSIGN_VEHICLE, "ASSIGN_VEHICLE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(ASSIGN_PED, "ASSIGN_PED", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(MASS_PACKET_SEQUENCE, "MASS_PACKET_SEQUENCE", PacketDirection::ServerToClient, PacketReliability::Mixed, 0);
		set(CUTSCENE_SKIP_VOTE_UPDATE, "CUTSCENE_SKIP_VOTE_UPDATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(FIRE_CREATE, "FIRE_CREATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(FIRE_UPDATE, "FIRE_UPDATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(FIRE_REMOVE, "FIRE_REMOVE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_SNAPSHOT_BEGIN, "PICKUP_SNAPSHOT_BEGIN", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_SNAPSHOT_ENTRY, "PICKUP_SNAPSHOT_ENTRY", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_SNAPSHOT_END, "PICKUP_SNAPSHOT_END", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_STATE_DELTA, "PICKUP_STATE_DELTA", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_DROP_CREATE, "PICKUP_DROP_CREATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PICKUP_DROP_RESOLVE, "PICKUP_DROP_RESOLVE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PROPERTY_STATE_SNAPSHOT_BEGIN, "PROPERTY_STATE_SNAPSHOT_BEGIN", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PROPERTY_STATE_SNAPSHOT_ENTRY, "PROPERTY_STATE_SNAPSHOT_ENTRY", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PROPERTY_STATE_SNAPSHOT_END, "PROPERTY_STATE_SNAPSHOT_END", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(SUBMISSION_MISSION_SNAPSHOT_BEGIN, "SUBMISSION_MISSION_SNAPSHOT_BEGIN", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(SUBMISSION_MISSION_SNAPSHOT_ENTRY, "SUBMISSION_MISSION_SNAPSHOT_ENTRY", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(SUBMISSION_MISSION_SNAPSHOT_END, "SUBMISSION_MISSION_SNAPSHOT_END", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);
		set(PLAYER_SNIPER_AIM_MARKER_STATE, "PLAYER_SNIPER_AIM_MARKER_STATE", PacketDirection::ServerToClient, PacketReliability::Reliable, 0);

		// client -> server
		set(PED_CLAIM_ON_RELEASE, "PED_CLAIM_ON_RELEASE", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);
		set(PED_CANCEL_CLAIM, "PED_CANCEL_CLAIM", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);
		set(PED_TAKE_HOST, "PED_TAKE_HOST", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);
		set(PICKUP_SNAPSHOT_REQUEST, "PICKUP_SNAPSHOT_REQUEST", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);
		set(PICKUP_COLLECT_REQUEST, "PICKUP_COLLECT_REQUEST", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);
		set(CHEAT_EFFECT_TRIGGER, "CHEAT_EFFECT_TRIGGER", PacketDirection::ClientToServer, PacketReliability::Reliable, 0);

		return contracts;
	}

	inline const std::array<PacketContract, PACKET_ID_MAX>& Contracts()
	{
		static const std::array<PacketContract, PACKET_ID_MAX> contracts = BuildContracts();
		return contracts;
	}

	inline bool ShouldHaveListener(ReceiverRole role, PacketDirection direction)
	{
		if (role == ReceiverRole::Client)
		{
			return direction == PacketDirection::ServerToClient || direction == PacketDirection::Bidirectional;
		}
		return direction == PacketDirection::ClientToServer || direction == PacketDirection::Bidirectional;
	}

	inline ListenerValidationReport ValidateListeners(ReceiverRole role, const std::unordered_set<unsigned short>& registeredIds)
	{
		ListenerValidationReport report{};
		for (const PacketContract& contract : Contracts())
		{
			if (ShouldHaveListener(role, contract.direction))
			{
				++report.requiredCount;
				if (registeredIds.find(contract.id) == registeredIds.end())
				{
					report.ok = false;
					++report.missingCount;
					report.details += "  missing: ";
					report.details += contract.name;
					report.details += "\n";
				}
			}
		}

		for (unsigned short id : registeredIds)
		{
			if (id >= PACKET_ID_MAX)
			{
				report.ok = false;
				++report.unexpectedCount;
				report.details += "  invalid-id: ";
				report.details += std::to_string(id);
				report.details += "\n";
				continue;
			}

			const PacketContract& contract = Contracts()[id];
			if (!ShouldHaveListener(role, contract.direction))
			{
				report.ok = false;
				++report.unexpectedCount;
				report.details += "  unexpected: ";
				report.details += contract.name;
				report.details += "\n";
			}
		}

		report.registeredCount = registeredIds.size();
		return report;
	}
}
