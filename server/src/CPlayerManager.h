#pragma once

#ifndef _CPLAYERMANAGER_H_
	#define _CPLAYERMANAGER_H_
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <climits>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <chrono>

#include "CControllerState.h"
#include "CPlayer.h"
#include "NetworkEntityType.h"
#include "PlayerDisconnectReason.h"
#include "ConfigManager.h"
#include "CPickupManager.h"
#include "CFireSyncManager.h"
#include "CMissionRuntimeManager.h"

class CPlayerManager
{
	public:
		CPlayerManager();
		
		static std::vector<CPlayer*> m_pPlayers;
		static void Add(CPlayer* player);
		static void Remove(CPlayer* player);
		static CPlayer* GetPlayer(int playerid);
		static CPlayer* GetPlayer(ENetPeer* peer);
		static int GetFreeID();
		static CPlayer* GetHost();
		static void AssignHostToFirstPlayer();
		
		~CPlayerManager();
};


class CPlayerPackets
{
public:
	CPlayerPackets();
	static void SendPickupBootstrap(ENetPeer* peer)
	{
		if (!peer)
		{
			return;
		}

		CPickupManager::SendSnapshot(peer);
	}

	static void BroadcastPickupBootstrapResync()
	{
		for (auto* player : CPlayerManager::m_pPlayers)
		{
			if (player && player->m_pPeer)
			{
				SendPickupBootstrap(player->m_pPeer);
			}
		}
	}

	static void BroadcastPropertyStateResync()
	{
		for (auto* player : CPlayerManager::m_pPlayers)
		{
			if (player && player->m_pPeer)
			{
				SendPropertyStateSnapshot(player->m_pPeer);
			}
		}
	}

	static void BroadcastSubmissionMissionStateResync()
	{
		for (auto* player : CPlayerManager::m_pPlayers)
		{
			if (player && player->m_pPeer)
			{
				SendSubmissionMissionStateSnapshot(player->m_pPeer);
			}
		}
	}

	static void BroadcastMissionRuntimeStateResync()
	{
		for (auto* player : CPlayerManager::m_pPlayers)
		{
			if (player && player->m_pPeer)
			{
				CMissionRuntimeManager::SendSnapshotTo(player->m_pPeer);
			}
		}
	}

	struct GangZoneState
	{
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t state;
		uint8_t currArea;
	};

	struct GangGroupMembershipState
	{
		uint32_t sequence;
		int pedNetworkId;
		uint8_t gangGroupId;
		uint8_t action;
	};

	struct GangRelationshipState
	{
		uint32_t sequence;
		uint8_t sourceGangGroupId;
		uint8_t targetGangGroupId;
		uint8_t relationshipFlags;
	};

	struct GangWarLifecycleState
	{
		uint32_t sequence;
		uint8_t eventType;
		uint8_t warState;
		uint8_t warPhase;
		uint8_t outcome;
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t zoneState;
		uint8_t currArea;
	};

	struct MapPinState
	{
		int playerid;
		bool place;
		CVector position;
		uint8_t currArea;
	};

	struct PropertyState
	{
		uint16_t propertyId = 0;
		int ownerPlayerId = -1;
		uint8_t unlocked = 0;
		uint8_t linkedPickupActive = 0;
		uint8_t linkedInteriorUnlocked = 0;
		uint8_t currArea = 0;
		uint64_t stateTimestampMs = 0;
		uint32_t stateVersion = 0;
	};

	struct SubmissionMissionState
	{
		uint8_t submissionType = 0;
		uint8_t active = 0;
		uint8_t level = 0;
		uint8_t stage = 0;
		uint16_t progress = 0;
		int32_t timerMs = 0;
		int32_t score = 0;
		int32_t rewardCash = 0;
		uint8_t outcome = 0;
		uint8_t participantCount = 0;
		uint8_t currArea = 0;
		uint64_t stateTimestampMs = 0;
		uint32_t stateVersion = 0;
	};

	static inline std::unordered_map<uint16_t, GangZoneState> ms_gangZoneStates{};
	static inline std::unordered_map<int, GangGroupMembershipState> ms_gangGroupMembershipStates{};
	static inline std::unordered_map<uint16_t, GangRelationshipState> ms_gangRelationshipStates{};
	static inline GangWarLifecycleState ms_lastGangWarLifecycleState{};
	static inline std::vector<MapPinState> ms_activeMapPins{};
	static inline std::unordered_map<uint16_t, PropertyState> ms_propertyStates{};
	static inline uint32_t ms_propertySnapshotVersion = 0;
	static inline std::unordered_map<uint8_t, SubmissionMissionState> ms_submissionMissionStates{};
	static inline uint32_t ms_submissionMissionSnapshotVersion = 0;

	static void RefreshActiveMapPinsSnapshot()
	{
		ms_activeMapPins.clear();
		for (auto* connectedPlayer : CPlayerManager::m_pPlayers)
		{
			if (!connectedPlayer || !connectedPlayer->m_ucSyncFlags.bWaypointModified)
			{
				continue;
			}

			MapPinState waypointPacket{};
			waypointPacket.playerid = connectedPlayer->m_iPlayerId;
			waypointPacket.place = true;
			waypointPacket.position = connectedPlayer->m_vecWaypointPos;
			waypointPacket.currArea = connectedPlayer->m_nCurrArea;
			ms_activeMapPins.push_back(waypointPacket);
		}
	}

	static void SendMapStateSnapshot(ENetPeer* peer)
	{
		if (!peer)
		{
			return;
		}

		for (const auto& [zoneId, zoneState] : ms_gangZoneStates)
		{
			CNetwork::SendPacket(peer, CPacketsID::GANG_ZONE_STATE, (void*)&zoneState, sizeof(zoneState), ENET_PACKET_FLAG_RELIABLE);
		}

		for (const auto& [pedNetworkId, membershipState] : ms_gangGroupMembershipStates)
		{
			CNetwork::SendPacket(peer, CPacketsID::GANG_GROUP_MEMBERSHIP_UPDATE, (void*)&membershipState, sizeof(membershipState), ENET_PACKET_FLAG_RELIABLE);
		}

		for (const auto& [relationshipKey, relationshipState] : ms_gangRelationshipStates)
		{
			CNetwork::SendPacket(peer, CPacketsID::GANG_RELATIONSHIP_UPDATE, (void*)&relationshipState, sizeof(relationshipState), ENET_PACKET_FLAG_RELIABLE);
		}

		if (ms_lastGangWarLifecycleState.sequence != 0)
		{
			CNetwork::SendPacket(peer, CPacketsID::GANG_WAR_LIFECYCLE_EVENT, (void*)&ms_lastGangWarLifecycleState, sizeof(ms_lastGangWarLifecycleState), ENET_PACKET_FLAG_RELIABLE);
		}

		RefreshActiveMapPinsSnapshot();
		for (auto& waypointPacket : ms_activeMapPins)
		{
			CNetwork::SendPacket(peer, CPacketsID::PLAYER_PLACE_WAYPOINT, &waypointPacket, sizeof(waypointPacket), ENET_PACKET_FLAG_RELIABLE);
		}
	}

	static void SendMissionStateSnapshot(ENetPeer* peer)
	{
		CMissionRuntimeManager::SendSnapshotTo(peer);
	}

	static void SendPropertyStateSnapshot(ENetPeer* peer)
	{
		if (!peer)
		{
			return;
		}

		CPropertyStatePackets::PropertyStateSnapshotBegin beginPacket{};
		beginPacket.snapshotVersion = ms_propertySnapshotVersion;
		beginPacket.propertyCount = static_cast<uint16_t>(std::min<size_t>(ms_propertyStates.size(), UINT16_MAX));
		CNetwork::SendPacket(peer, CPacketsID::PROPERTY_STATE_SNAPSHOT_BEGIN, &beginPacket, sizeof(beginPacket), ENET_PACKET_FLAG_RELIABLE);

		for (const auto& [propertyId, state] : ms_propertyStates)
		{
			CPropertyStatePackets::PropertyStateSnapshotEntry entry{};
			entry.propertyId = propertyId;
			entry.ownerPlayerId = state.ownerPlayerId;
			entry.unlocked = state.unlocked;
			entry.linkedPickupActive = state.linkedPickupActive;
			entry.linkedInteriorUnlocked = state.linkedInteriorUnlocked;
			entry.currArea = state.currArea;
			entry.stateTimestampMs = state.stateTimestampMs;
			entry.stateVersion = state.stateVersion;
			CNetwork::SendPacket(peer, CPacketsID::PROPERTY_STATE_SNAPSHOT_ENTRY, &entry, sizeof(entry), ENET_PACKET_FLAG_RELIABLE);
		}

		CPropertyStatePackets::PropertyStateSnapshotEnd endPacket{};
		endPacket.snapshotVersion = ms_propertySnapshotVersion;
		CNetwork::SendPacket(peer, CPacketsID::PROPERTY_STATE_SNAPSHOT_END, &endPacket, sizeof(endPacket), ENET_PACKET_FLAG_RELIABLE);
	}

	static void SendSubmissionMissionStateSnapshot(ENetPeer* peer)
	{
		if (!peer)
		{
			return;
		}

		CSubmissionMissionPackets::SubmissionMissionSnapshotBegin beginPacket{};
		beginPacket.snapshotVersion = ms_submissionMissionSnapshotVersion;
		beginPacket.submissionCount = static_cast<uint8_t>(std::min<size_t>(ms_submissionMissionStates.size(), UINT8_MAX));
		CNetwork::SendPacket(peer, CPacketsID::SUBMISSION_MISSION_SNAPSHOT_BEGIN, &beginPacket, sizeof(beginPacket), ENET_PACKET_FLAG_RELIABLE);

		for (const auto& [submissionType, state] : ms_submissionMissionStates)
		{
			CSubmissionMissionPackets::SubmissionMissionSnapshotEntry entry{};
			entry.submissionType = submissionType;
			entry.active = state.active ? 1 : 0;
			entry.level = state.level;
			entry.stage = state.stage;
			entry.progress = state.progress;
			entry.timerMs = state.timerMs;
			entry.score = state.score;
			entry.rewardCash = state.rewardCash;
			entry.outcome = state.outcome;
			entry.participantCount = state.participantCount;
			entry.currArea = state.currArea;
			entry.stateTimestampMs = state.stateTimestampMs;
			entry.stateVersion = state.stateVersion;
			CNetwork::SendPacket(peer, CPacketsID::SUBMISSION_MISSION_SNAPSHOT_ENTRY, &entry, sizeof(entry), ENET_PACKET_FLAG_RELIABLE);
		}

		CSubmissionMissionPackets::SubmissionMissionSnapshotEnd endPacket{};
		endPacket.snapshotVersion = ms_submissionMissionSnapshotVersion;
		CNetwork::SendPacket(peer, CPacketsID::SUBMISSION_MISSION_SNAPSHOT_END, &endPacket, sizeof(endPacket), ENET_PACKET_FLAG_RELIABLE);
	}
	struct CutsceneSkipVoteState
	{
		bool active = false;
		bool alreadySkipped = false;
		char cutsceneName[8]{};
		uint32_t sessionToken = 0;
		std::unordered_set<int> eligiblePlayerIds{};
		std::unordered_set<int> votedPlayerIds{};
		int voteCount = 0;
		int requiredVotes = 0;
		uint64_t voteUnlockTimeMs = 0;
	};

	struct CutsceneSkipVoteConfig
	{
		double thresholdRatio = 0.51;
		int minimumPlayers = 2;
		uint32_t lockMs = 0;
	};

	struct CutsceneSkipVoteUpdate
	{
		int voterid;
		int currentVotes;
		int requiredVotes;
		uint32_t sessionToken;
		bool thresholdReached;
	};

	static CutsceneSkipVoteState ms_cutsceneSkipVoteState;
	static CutsceneSkipVoteConfig ms_cutsceneSkipVoteConfig;
	static bool ms_isCutsceneSkipVoteConfigLoaded;

	static void EnsureCutsceneSkipVoteConfigLoaded()
	{
		if (ms_isCutsceneSkipVoteConfigLoaded)
		{
			return;
		}

		ms_cutsceneSkipVoteConfig.thresholdRatio = CConfigManager::GetCutsceneSkipVoteThresholdRatio();
		ms_cutsceneSkipVoteConfig.minimumPlayers = CConfigManager::GetCutsceneSkipVoteMinPlayers();
		ms_cutsceneSkipVoteConfig.lockMs = CConfigManager::GetCutsceneSkipVoteLockMs();
		ms_isCutsceneSkipVoteConfigLoaded = true;
	}

	static uint64_t GetServerTimeMs()
	{
		const auto now = std::chrono::steady_clock::now();
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
		return (uint64_t)ms.count();
	}

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
		float stats[CPlayer::PLAYER_STATS_SYNCED_COUNT]{};
		int money = 0;
	};

	struct CheatEffectThrottleState
	{
		uint64_t windowStartMs = 0;
		uint8_t eventCount = 0;
	};

	static inline std::unordered_map<ENetPeer*, CheatEffectThrottleState> ms_cheatEffectThrottleByPeer{};

	static bool IsCheatEffectRateLimited(ENetPeer* peer)
	{
		constexpr uint64_t THROTTLE_WINDOW_MS = 1000;
		constexpr uint8_t THROTTLE_MAX_EVENTS_PER_WINDOW = 8;

		auto& state = ms_cheatEffectThrottleByPeer[peer];
		const uint64_t nowMs = GetServerTimeMs();
		if (state.windowStartMs == 0 || nowMs < state.windowStartMs || (nowMs - state.windowStartMs) >= THROTTLE_WINDOW_MS)
		{
			state.windowStartMs = nowMs;
			state.eventCount = 0;
		}

		if (state.eventCount >= THROTTLE_MAX_EVENTS_PER_WINDOW)
		{
			return true;
		}

		state.eventCount++;
		return false;
	}

	static void ResetCheatEffectThrottle(ENetPeer* peer)
	{
		if (!peer)
		{
			return;
		}

		ms_cheatEffectThrottleByPeer.erase(peer);
	}

	static void RecalculateCutsceneVoteThreshold()
	{
		auto& state = ms_cutsceneSkipVoteState;
		int eligiblePlayers = (int)state.eligiblePlayerIds.size();
		if (eligiblePlayers <= 0)
		{
			state.requiredVotes = 0;
			return;
		}

		double configuredRatio = std::clamp(ms_cutsceneSkipVoteConfig.thresholdRatio, 0.0, 1.0);
		int requiredVotes = (int)std::ceil((double)eligiblePlayers * configuredRatio);
		if (requiredVotes <= 0)
		{
			requiredVotes = (eligiblePlayers / 2) + 1;
		}

		state.requiredVotes = std::clamp(requiredVotes, 1, eligiblePlayers);
	}

	static void BroadcastCutsceneVoteUpdate(int voterId)
	{
		auto& state = ms_cutsceneSkipVoteState;
		CutsceneSkipVoteUpdate packet{};
		packet.voterid = voterId;
		packet.currentVotes = state.voteCount;
		packet.requiredVotes = state.requiredVotes;
		packet.sessionToken = state.sessionToken;
		packet.thresholdReached = state.alreadySkipped;
		CNetwork::SendPacketToAll(CPacketsID::CUTSCENE_SKIP_VOTE_UPDATE, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE, nullptr);
	}

	static void ClearCutsceneVoteState()
	{
		ms_cutsceneSkipVoteState = {};
	}

	static void ResetCutsceneVoteStateForHostMigration()
	{
		ClearCutsceneVoteState();
	}

	static void OnPlayerDisconnectedFromCutsceneVote(int playerId)
	{
		auto& state = ms_cutsceneSkipVoteState;
		if (!state.active)
		{
			return;
		}

		if (state.eligiblePlayerIds.erase(playerId) > 0)
		{
			if (state.votedPlayerIds.erase(playerId) > 0)
			{
				state.voteCount = std::max(0, state.voteCount - 1);
			}

			RecalculateCutsceneVoteThreshold();
			BroadcastCutsceneVoteUpdate(-1);

			if (!state.alreadySkipped
				&& (int)state.eligiblePlayerIds.size() >= ms_cutsceneSkipVoteConfig.minimumPlayers
				&& state.voteCount >= state.requiredVotes)
			{
				state.alreadySkipped = true;
				struct RawSkipCutscenePacket
				{
					int playerid;
					int votes;
					uint32_t sessionToken;
				} skipPacket{};
				skipPacket.playerid = -1;
				skipPacket.votes = state.voteCount;
				skipPacket.sessionToken = state.sessionToken;
				CNetwork::SendPacketToAll(CPacketsID::SKIP_CUTSCENE, &skipPacket, sizeof(skipPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
				ClearCutsceneVoteState();
			}
		}
	}
#pragma pack(1)
	struct PlayerConnected
	{
		int id;
		bool isAlreadyConnected; // prevents spam in the chat when connecting by distinguishing already connected players from newly joined ones
		char name[32 + 1];
		uint32_t version;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (CPlayerManager::GetPlayer(peer))
			{
				return;
			}

			CNetwork::HandlePlayerConnected(peer, data, size);
		}
	};

	struct PlayerDisconnected
	{
		int id;
		ePlayerDisconnectReason reason;
		uint32_t version;
	};

	struct PickupSnapshotRequest : public CPickupStatePackets::PickupSnapshotRequest
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (!CPlayerManager::GetPlayer(peer))
			{
				return;
			}

			SendPickupBootstrap(peer);
		}
	};

	struct PropertyStateDelta : public CPropertyStatePackets::PropertyStateDelta
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost || size < (int)sizeof(CPropertyStatePackets::PropertyStateDelta))
			{
				return;
			}

			auto* packet = (CPropertyStatePackets::PropertyStateDelta*)data;
			packet->action = std::min<uint8_t>(packet->action, CPropertyStatePackets::PROPERTY_STATE_ACTION_REMOVE);

			if (packet->action == CPropertyStatePackets::PROPERTY_STATE_ACTION_REMOVE)
			{
				ms_propertyStates.erase(packet->propertyId);
			}
			else
			{
				PropertyState state{};
				state.propertyId = packet->propertyId;
				state.ownerPlayerId = packet->ownerPlayerId;
				state.unlocked = packet->unlocked ? 1 : 0;
				state.linkedPickupActive = packet->linkedPickupActive ? 1 : 0;
				state.linkedInteriorUnlocked = packet->linkedInteriorUnlocked ? 1 : 0;
				state.currArea = packet->currArea;
				state.stateTimestampMs = packet->stateTimestampMs;
				state.stateVersion = packet->stateVersion;

				auto it = ms_propertyStates.find(packet->propertyId);
				if (it != ms_propertyStates.end() && state.stateVersion < it->second.stateVersion)
				{
					return;
				}

				ms_propertyStates[packet->propertyId] = state;
			}

			ms_propertySnapshotVersion++;
			CNetwork::SendPacketToAll(CPacketsID::PROPERTY_STATE_DELTA, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct SubmissionMissionStateDelta : public CSubmissionMissionPackets::SubmissionMissionStateDelta
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost || size < (int)sizeof(CSubmissionMissionPackets::SubmissionMissionStateDelta))
			{
				return;
			}

			auto* packet = (CSubmissionMissionPackets::SubmissionMissionStateDelta*)data;
			packet->submissionType = std::min<uint8_t>(packet->submissionType, CSubmissionMissionPackets::SUBMISSION_MISSION_TYPE_MAX - 1);
			packet->action = std::min<uint8_t>(packet->action, CSubmissionMissionPackets::SUBMISSION_MISSION_ACTION_CLEAR);

			if (packet->action == CSubmissionMissionPackets::SUBMISSION_MISSION_ACTION_CLEAR)
			{
				ms_submissionMissionStates.erase(packet->submissionType);
			}
			else
			{
				SubmissionMissionState state{};
				state.submissionType = packet->submissionType;
				state.active = packet->active ? 1 : 0;
				state.level = packet->level;
				state.stage = packet->stage;
				state.progress = packet->progress;
				state.timerMs = packet->timerMs;
				state.score = packet->score;
				state.rewardCash = packet->rewardCash;
				state.outcome = packet->outcome;
				state.participantCount = packet->participantCount;
				state.currArea = packet->currArea;
				state.stateTimestampMs = packet->stateTimestampMs;
				state.stateVersion = packet->stateVersion;

				auto it = ms_submissionMissionStates.find(packet->submissionType);
				if (it != ms_submissionMissionStates.end() && state.stateVersion < it->second.stateVersion)
				{
					return;
				}

				ms_submissionMissionStates[packet->submissionType] = state;
			}

			ms_submissionMissionSnapshotVersion++;
			CNetwork::SendPacketToAll(CPacketsID::SUBMISSION_MISSION_STATE_DELTA, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

#pragma pack(1)
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				// create packet
				CPlayerPackets::PlayerOnFoot* packet = (CPlayerPackets::PlayerOnFoot*)data;

				// set packet`s playerid, cuz incoming packet has id = 0
				packet->id = player->m_iPlayerId;

				bool isValidWeapon = (packet->weapon >= 0 && packet->weapon <= 18) || (packet->weapon >= 22 && packet->weapon <= 46);
				if (!isValidWeapon)
				{
					packet->weapon = 0;
					packet->ammo = 0;
				}

				if (packet->fightingStyle < 4 || packet->fightingStyle > 16)
				{
					packet->fightingStyle = 4;
				}

				if (packet->velocity.x > 10.0f || packet->velocity.y > 10.0f || packet->velocity.z > 10.0f)
				{
					packet->velocity = CVector(0.0f, 0.0f, 0.0f);
				}

				if (player->m_nVehicleId >= 0)
				{
					player->RemoveFromVehicle();
				}

				player->m_vecPosition = packet->position;
				player->m_ucCurrentWeapon = packet->weapon;
				player->m_usCurrentAmmo = packet->ammo;
				player->m_bHasJetpack = packet->hasJetpack;

				CNetwork::SendPacketToAll(CPacketsID::PLAYER_ONFOOT, packet, sizeof * packet, (ENetPacketFlag)0, peer);
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player)
			{
				return;
			}

			auto* packet = (CPlayerPackets::PlayerJetpackTransition*)data;
			packet->playerid = player->m_iPlayerId;

			const bool previousState = player->m_bHasJetpack;
			const bool nextState = packet->hasJetpack;
			const bool duplicatedAcquire = previousState && packet->intent == JETPACK_TRANSITION_ACQUIRE;
			const bool staleRemove = !previousState && (packet->intent == JETPACK_TRANSITION_REMOVE || packet->intent == JETPACK_TRANSITION_FORCED_REMOVE);

			printf("[JetpackTransition][Server][Recv] player=%d intent=%u prev=%d next=%d duplicateAcquire=%d staleRemove=%d\n",
				packet->playerid,
				packet->intent,
				previousState ? 1 : 0,
				nextState ? 1 : 0,
				duplicatedAcquire ? 1 : 0,
				staleRemove ? 1 : 0);

			player->m_bHasJetpack = nextState;
			CNetwork::SendPacketToAll(CPacketsID::PLAYER_JETPACK_TRANSITION, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

#pragma pack(1)
	struct PlayerBulletShot
	{
		int playerid;
		int targetid;
		CVector startPos;
		CVector endPos;
		unsigned char colPoint[44]; // padding
		int incrementalHit;
		unsigned char entityType;
		unsigned char shotWeaponId;
		bool moonSniperActive;
		float shotSizeMultiplier;

		static bool IsMoonSniperShot(const CPlayerPackets::PlayerBulletShot& packet)
		{
			constexpr uint8_t SNIPER_WEAPON_ID = 34;
			const CVector delta = packet.endPos - packet.startPos;
			const float horizontalDistance = std::sqrt((delta.x * delta.x) + (delta.y * delta.y));
			return packet.shotWeaponId == SNIPER_WEAPON_ID
				&& delta.z > 120.0f
				&& horizontalDistance > 250.0f;
		}

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			// create packet
			CPlayerPackets::PlayerBulletShot* packet = (CPlayerPackets::PlayerBulletShot*)data;
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player)
			{
				return;
			}

			// set packet`s playerid, cuz incoming packet has id = 0
			packet->playerid = player->m_iPlayerId;
			packet->shotWeaponId = player->m_ucCurrentWeapon;
			packet->moonSniperActive = IsMoonSniperShot(*packet);
			packet->shotSizeMultiplier = packet->moonSniperActive ? 1.75f : 1.0f;

			CNetwork::SendPacketToAll(CPacketsID::PLAYER_BULLET_SHOT, packet, sizeof * packet, (ENetPacketFlag)0, peer);
		}
	};

	struct PlayerSniperAimMarkerState
	{
		int playerid;
		CVector source;
		CVector direction;
		float range;
		bool visible;
		uint32_t tick;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost || size != sizeof(CPlayerPackets::PlayerSniperAimMarkerState))
			{
				return;
			}

			auto* packet = (CPlayerPackets::PlayerSniperAimMarkerState*)data;
			packet->playerid = std::clamp(packet->playerid, 0, MAX_SERVER_PLAYERS - 1);
			packet->direction.Normalise();
			packet->range = std::clamp(packet->range, 0.0f, 1200.0f);

			auto* owner = CPlayerManager::m_pPlayers[packet->playerid];
			if (!owner)
			{
				return;
			}

			owner->m_vecSniperAimSource = packet->source;
			owner->m_vecSniperAimDirection = packet->direction;
			owner->m_fSniperAimRange = packet->range;
			owner->m_bSniperAimVisible = packet->visible;
			owner->m_nSniperAimTick = packet->tick;

			CNetwork::SendPacketToAll(CPacketsID::PLAYER_SNIPER_AIM_MARKER_STATE, packet, sizeof(*packet), (ENetPacketFlag)0, peer);
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				CPlayerPackets::PlayerPlaceWaypoint* packet = (CPlayerPackets::PlayerPlaceWaypoint*)data;
				packet->playerid = player->m_iPlayerId;
				packet->position.x = std::clamp(packet->position.x, -3000.0f, 3000.0f);
				packet->position.y = std::clamp(packet->position.y, -3000.0f, 3000.0f);
				packet->currArea = player->m_nCurrArea;
				player->m_ucSyncFlags.bWaypointModified = packet->place != 0;
				player->m_vecWaypointPos = packet->position;
				CNetwork::SendPacketToAll(CPacketsID::PLAYER_PLACE_WAYPOINT, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);
			}
		}
	};

	struct PlayerSetHost
	{
		int playerid;
		uint32_t missionEpoch;
	};

	struct AddExplosion
	{
		unsigned char type;
		CVector pos;
		int time;
		bool usesSound;
		float cameraShake;
		bool isVisible;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				CPlayerPackets::AddExplosion* packet = (CPlayerPackets::AddExplosion*)data;
				packet->pos.x = std::clamp(packet->pos.x, -4000.0f, 4000.0f);
				packet->pos.y = std::clamp(packet->pos.y, -4000.0f, 4000.0f);
				packet->pos.z = std::clamp(packet->pos.z, -200.0f, 2000.0f);
				packet->type = std::min<uint8_t>(packet->type, 12);
				packet->time = std::clamp(packet->time, 0, 15000);
				packet->cameraShake = std::clamp(packet->cameraShake, 0.0f, 500.0f);
				CNetwork::SendPacketToAll(CPacketsID::ADD_EXPLOSION, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);
				CFireSyncManager::OnExplosion(player, packet->pos, packet->type);
			}
		}
	};

	struct PlayerChatMessage
	{
		int playerid;
		wchar_t message[128 + 1];

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CPlayerPackets::PlayerChatMessage* packet = (CPlayerPackets::PlayerChatMessage*)data;
			packet->playerid = CPlayerManager::GetPlayer(peer)->m_iPlayerId;
			packet->message[128] = 0;
			CNetwork::SendPacketToAll(CPacketsID::PLAYER_CHAT_MESSAGE, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct PlayerKeySync
	{
		int playerid;
		CCompressedControllerState newState;
		uint8_t currArea;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CPlayerPackets::PlayerKeySync* packet = (CPlayerPackets::PlayerKeySync*)data;
			auto* player = CPlayerManager::GetPlayer(peer);
			packet->playerid = player->m_iPlayerId;
			player->m_nCurrArea = packet->currArea;
			CNetwork::SendPacketToAll(CPacketsID::PLAYER_KEY_SYNC, packet, sizeof * packet, (ENetPacketFlag)0, peer);
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (!CPlayerManager::GetPlayer(peer)->m_bIsHost)
				return;

			CPlayerPackets::GameWeatherTime* packet = (CPlayerPackets::GameWeatherTime*)data;
			CNetwork::SendPacketToAll(CPacketsID::GAME_WEATHER_TIME, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct CheatEffectTriggerPacket
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost)
			{
				return;
			}

			if (size != sizeof(CPlayerPackets::CheatEffectTrigger))
			{
				return;
			}

			if (IsCheatEffectRateLimited(peer))
			{
				return;
			}

			auto* packet = (CPlayerPackets::CheatEffectTrigger*)data;
			switch (packet->effectType)
			{
				case CHEAT_EFFECT_WORLD_WEATHER_TIME:
				{
					CPlayerPackets::GameWeatherTime weatherPacket{};
					weatherPacket.newWeather = packet->newWeather;
					weatherPacket.oldWeather = packet->oldWeather;
					weatherPacket.forcedWeather = packet->forcedWeather;
					weatherPacket.currentMonth = packet->currentMonth;
					weatherPacket.currentDay = packet->currentDay;
					weatherPacket.currentHour = packet->currentHour;
					weatherPacket.currentMinute = packet->currentMinute;
					weatherPacket.gameTickCount = packet->gameTickCount;
					CPlayerPackets::GameWeatherTime::Handle(peer, &weatherPacket, sizeof(weatherPacket));
					break;
				}
				case CHEAT_EFFECT_PLAYER_WANTED_LEVEL:
				{
					CPlayerPackets::PlayerWantedLevel wantedPacket{};
					wantedPacket.wantedLevel = std::min<uint8_t>(packet->wantedLevel, 6);
					CPlayerPackets::PlayerWantedLevel::Handle(peer, &wantedPacket, sizeof(wantedPacket));
					break;
				}
				case CHEAT_EFFECT_PLAYER_STATS:
				{
					CPlayerPackets::PlayerStats statsPacket{};
					memcpy(statsPacket.stats, packet->stats, sizeof(statsPacket.stats));
					statsPacket.money = std::clamp(packet->money, 0, INT_MAX);
					CPlayerPackets::PlayerStats::Handle(peer, &statsPacket, sizeof(statsPacket));
					break;
				}
				default:
					return;
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player)
			{
				return;
			}

			CPlayerPackets::PlayerAimSync* packet = (CPlayerPackets::PlayerAimSync*)data;
			packet->playerid = player->m_iPlayerId;
			CNetwork::SendPacketToAll(CPacketsID::PLAYER_AIM_SYNC, packet, sizeof * packet, (ENetPacketFlag)0, peer);

			CPlayerPackets::PlayerSniperAimMarkerState markerPacket{};
			markerPacket.playerid = player->m_iPlayerId;
			markerPacket.source = packet->source;
			markerPacket.direction = packet->front;
			markerPacket.direction.Normalise();
			markerPacket.range = 450.0f;
			markerPacket.visible = player->m_ucCurrentWeapon == 34;
			markerPacket.tick = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();

			player->m_vecSniperAimSource = markerPacket.source;
			player->m_vecSniperAimDirection = markerPacket.direction;
			player->m_fSniperAimRange = markerPacket.range;
			player->m_bSniperAimVisible = markerPacket.visible;
			player->m_nSniperAimTick = markerPacket.tick;

			CNetwork::SendPacketToAll(CPacketsID::PLAYER_SNIPER_AIM_MARKER_STATE, &markerPacket, sizeof(markerPacket), (ENetPacketFlag)0, peer);
		}
	};

	struct PlayerWantedLevel
	{
		int playerid;
		uint8_t wantedLevel;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				auto* packet = (CPlayerPackets::PlayerWantedLevel*)data;
				const uint8_t clampedWantedLevel = std::min<uint8_t>(packet->wantedLevel, 6);
				if (player->m_nWantedLevel == clampedWantedLevel)
				{
					return;
				}

				player->m_nWantedLevel = clampedWantedLevel;
				player->m_ucSyncFlags.bWantedLevelModified = true;
				packet->playerid = player->m_iPlayerId;
				packet->wantedLevel = clampedWantedLevel;
				CNetwork::SendPacketToAll(CPacketsID::PLAYER_WANTED_LEVEL, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
			}
		}
	};

	struct PlayerStats
	{
		int playerid;
		float stats[CPlayer::PLAYER_STATS_SYNCED_COUNT];
		int money;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				CPlayerPackets::PlayerStats* packet = (CPlayerPackets::PlayerStats*)data;
				packet->playerid = player->m_iPlayerId;
				packet->money = std::clamp(packet->money, 0, INT_MAX);
				CNetwork::SendPacketToAll(CPacketsID::PLAYER_STATS, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);

				memcpy(player->m_afStats, packet->stats, sizeof(packet->stats));
				player->m_nMoney = packet->money;
				player->m_ucSyncFlags.bStatsModified = true;
			}
		}
	};
	static_assert(sizeof(PlayerStats) == 64, "CPlayerPackets::PlayerStats layout mismatch");
	static_assert(sizeof(PlayerStats::stats) == sizeof(CPlayer::m_afStats), "Player stats forwarding size mismatch");

	struct RebuildPlayer
	{
		int playerid;
		// CPedClothesDesc inlined
		unsigned int m_anModelKeys[10];
		unsigned int m_anTextureKeys[18];
		float m_fFatStat;
		float m_fMuscleStat;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				CPlayerPackets::RebuildPlayer* packet = (CPlayerPackets::RebuildPlayer*)data;
				packet->playerid = player->m_iPlayerId;
				CNetwork::SendPacketToAll(CPacketsID::REBUILD_PLAYER, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);

				memcpy(player->m_anModelKeys, packet->m_anModelKeys, sizeof(player->m_anModelKeys));
				memcpy(player->m_anTextureKeys, packet->m_anTextureKeys, sizeof(player->m_anTextureKeys));
				player->m_fFatStat = packet->m_fFatStat;
				player->m_fMuscleStat = packet->m_fMuscleStat;
				player->m_ucSyncFlags.bClothesModified = true;
			}
		}
	};

	struct RespawnPlayer
	{
		int playerid;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CPlayerPackets::RespawnPlayer* packet = (CPlayerPackets::RespawnPlayer*)data;
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player)
			{
				return;
			}

			CPickupManager::CreateDropsForPlayerDeath(player);
			packet->playerid = player->m_iPlayerId;
			CNetwork::SendPacketToAll(CPacketsID::RESPAWN_PLAYER, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);

			if (player->m_nWantedLevel != 0)
			{
				player->m_nWantedLevel = 0;
				player->m_ucSyncFlags.bWantedLevelModified = true;

				CPlayerPackets::PlayerWantedLevel wantedPacket{};
				wantedPacket.playerid = player->m_iPlayerId;
				wantedPacket.wantedLevel = 0;
				CNetwork::SendPacketToAll(CPacketsID::PLAYER_WANTED_LEVEL, &wantedPacket, sizeof(wantedPacket), ENET_PACKET_FLAG_RELIABLE);
			}
		}
	};

	struct StartCutscene
	{
		char name[8];
		uint8_t currArea; // AKA interior
		uint32_t sessionToken;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					EnsureCutsceneSkipVoteConfigLoaded();
					CPlayerPackets::StartCutscene* packet = (CPlayerPackets::StartCutscene*)data;
					auto& state = ms_cutsceneSkipVoteState;
					state = {};
					state.active = true;
					memcpy(state.cutsceneName, packet->name, sizeof(state.cutsceneName));
					state.sessionToken = packet->sessionToken;
					state.voteUnlockTimeMs = GetServerTimeMs() + ms_cutsceneSkipVoteConfig.lockMs;

					for (auto connectedPlayer : CPlayerManager::m_pPlayers)
					{
						state.eligiblePlayerIds.insert(connectedPlayer->m_iPlayerId);
					}

					RecalculateCutsceneVoteThreshold();
					CNetwork::SendPacketToAll(CPacketsID::START_CUTSCENE, packet, sizeof * packet, ENET_PACKET_FLAG_RELIABLE, peer);
					BroadcastCutsceneVoteUpdate(-1);
				}
			}
		}
	};

	static bool IsHostAuthoritativeOpcode(uint16_t opcode)
	{
		switch (opcode)
		{
		case 0x00BA: // Text.PrintBig
		case 0x00BC: // Text.PrintNow
		case 0x00BF: // Clock.GetTimeOfDay
		case 0x00DF: // Char.IsInAnyCar
		case 0x00FE: // Char.LocateAnyMeans3D
		case 0x00FF: // Char.LocateOnFoot3D
		case 0x014E: // Hud.DisplayOnscreenTimer
		case 0x014F: // Hud.ClearOnscreenTimer
		case 0x0151: // Hud.ClearOnscreenCounter
		case 0x0164: // Blip.Remove
		case 0x0256: // Player.IsPlaying
		case 0x02A7: // Blip.AddSpriteForContactPoint
		case 0x0318: // register_mission_passed
		case 0x0396: // Hud.FreezeOnscreenTimer
		case 0x03C3: // Hud.DisplayOnscreenTimerWithString
		case 0x03C4: // Hud.DisplayOnscreenCounterWithString
		case 0x03C0: // Char.StoreCarIsInNoSave
		case 0x03E5: // Text.PrintHelp
		case 0x03EE: // Player.CanStartMission
		case 0x0417: // Mission.LoadAndLaunchInternal
		case 0x045C: // fail_current_mission
		case 0x04F7: // Hud.DisplayNthOnscreenCounterWithString
		case 0x0629: // Stat.SetInt
		case 0x0652: // Stat.GetInt
		case 0x0811: // Char.GetCarIsUsing
		case 0x0890: // Hud.SetTimerBeepCountdownTime
		case 0x08EC: // Car.GetClass
		case 0x08FB: // Checkpoint.SetType
		case 0x0956: // Game.FindMaxNumberOfGroupMembers
		case 0x0996: // Checkpoint.SetHeading
		case 0x096E: // Car.IsLowRider
		case 0x06D5: // Checkpoint.Create
		case 0x06D6: // Checkpoint.Delete
		case 0x07F3: // Checkpoint.SetCoords
			return true;
		default:
			return false;
		}
	}

	struct OpCodeSync
	{
		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !data || size < (int)sizeof(COpCodePackets::OpCodeSyncHeader))
			{
				return;
			}

			auto* packet = (COpCodePackets::OpCodeSyncHeader*)data;
			const int expectedMinSize = (int)sizeof(COpCodePackets::OpCodeSyncHeader)
				+ (int)packet->intParamCount * (int)sizeof(int)
				+ (int)packet->stringParamCount * (int)sizeof(uint8_t);
			if (expectedMinSize > size)
			{
				return;
			}

			const int stringMetaOffset = (int)sizeof(COpCodePackets::OpCodeSyncHeader) + (int)packet->intParamCount * (int)sizeof(int);
			const uint8_t* stringLengths = (const uint8_t*)((const uint8_t*)data + stringMetaOffset);
			int expectedSize = expectedMinSize;
			for (uint16_t i = 0; i < packet->stringParamCount; i++)
			{
				expectedSize += (int)stringLengths[i];
				if (expectedSize > size)
				{
					return;
				}
			}

			if (expectedSize != size)
			{
				return;
			}

			if (IsHostAuthoritativeOpcode(packet->opcode) && !player->m_bIsHost)
			{
				return;
			}

			CNetwork::SendPacketToAll(CPacketsID::OPCODE_SYNC, data, size, ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct SkipCutscene
	{
		int playerid;
		int votes;
		uint32_t sessionToken;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				EnsureCutsceneSkipVoteConfigLoaded();
				auto& state = ms_cutsceneSkipVoteState;
				auto* packet = (SkipCutscene*)data;

				if (!state.active || state.alreadySkipped)
				{
					return;
				}

				if (packet->sessionToken != state.sessionToken)
				{
					return;
				}

				if (state.eligiblePlayerIds.find(player->m_iPlayerId) == state.eligiblePlayerIds.end())
				{
					return;
				}

				if (GetServerTimeMs() < state.voteUnlockTimeMs)
				{
					return;
				}

				if (!state.votedPlayerIds.insert(player->m_iPlayerId).second)
				{
					return;
				}

				state.voteCount++;
				RecalculateCutsceneVoteThreshold();

				if ((int)state.eligiblePlayerIds.size() < ms_cutsceneSkipVoteConfig.minimumPlayers)
				{
					return;
				}

				BroadcastCutsceneVoteUpdate(player->m_iPlayerId);

				if (state.voteCount >= state.requiredVotes)
				{
					state.alreadySkipped = true;
					SkipCutscene skipPacket{};
					skipPacket.playerid = player->m_iPlayerId;
					skipPacket.votes = state.voteCount;
					skipPacket.sessionToken = state.sessionToken;
					CNetwork::SendPacketToAll(CPacketsID::SKIP_CUTSCENE, &skipPacket, sizeof(skipPacket), ENET_PACKET_FLAG_RELIABLE, nullptr);
					ClearCutsceneVoteState();
				}
			}
		}
	};

	struct OnMissionFlagSync
	{
		uint8_t bOnMission : 1;
		uint32_t missionEpoch;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CMissionRuntimeManager::HandleOnMissionFlagSync(CPlayerManager::GetPlayer(peer), peer, data, size);
		}
	};

	struct MissionFlowSync
	{
		CMissionRuntimeManager::MissionFlowPayload payload;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CMissionRuntimeManager::HandleMissionFlowSync(CPlayerManager::GetPlayer(peer), peer, data, size);
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					UpdateEntityBlip* packet = (UpdateEntityBlip*)data;

					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, CPacketsID::UPDATE_ENTITY_BLIP, data, sizeof(UpdateEntityBlip), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
	};

	struct RemoveEntityBlip
	{
		int playerid;
		eNetworkEntityType entityType;
		int entityId;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					RemoveEntityBlip* packet = (RemoveEntityBlip*)data;

					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, CPacketsID::REMOVE_ENTITY_BLIP, data, sizeof(RemoveEntityBlip), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					AddMessageGXT* packet = (AddMessageGXT*)data;

					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, CPacketsID::ADD_MESSAGE_GXT, data, sizeof(AddMessageGXT), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
	};

	struct RemoveMessageGXT
	{
		int playerid;
		char gxt[8];

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					RemoveMessageGXT* packet = (RemoveMessageGXT*)data;

					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, CPacketsID::REMOVE_MESSAGE_GXT, data, sizeof(RemoveMessageGXT), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
	};

	struct ClearEntityBlips
	{
		int playerid;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					ClearEntityBlips* packet = (ClearEntityBlips*)data;

					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, CPacketsID::CLEAR_ENTITY_BLIPS, data, sizeof(ClearEntityBlips), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
	};

	struct PlayMissionAudio
	{
		uint8_t slotid;
		int audioid;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					PlayMissionAudio* packet = (PlayMissionAudio*)data;
					CNetwork::SendPacketToAll(CPacketsID::PLAY_MISSION_AUDIO, packet, sizeof(PlayMissionAudio), ENET_PACKET_FLAG_RELIABLE, peer);
				}
			}
		}
	};

	struct UpdateCheckpoint
	{
		CMissionRuntimeManager::CheckpointPayload payload;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CMissionRuntimeManager::HandleCheckpointUpdate(CPlayerManager::GetPlayer(peer), peer, data, size);
		}
	};

	struct RemoveCheckpoint
	{
		CMissionRuntimeManager::CheckpointRemovePayload payload;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			CMissionRuntimeManager::HandleCheckpointRemove(CPlayerManager::GetPlayer(peer), peer, data, size);
		}
	};

	struct EnExSync
	{
		static inline std::vector<uint8_t> ms_vLastData;
		static inline CPlayer* ms_pLastPlayerOwner;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					ms_vLastData.assign((uint8_t*)data, (uint8_t*)data + size);
					ms_pLastPlayerOwner = player;

					CNetwork::SendPacketToAll(CPacketsID::ENEX_SYNC, data, size, ENET_PACKET_FLAG_RELIABLE, peer);
				}
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					CNetwork::SendPacketToAll(CPacketsID::CREATE_STATIC_BLIP, data, sizeof(CreateStaticBlip), ENET_PACKET_FLAG_RELIABLE, peer);
				}
			}
		}
	};

	struct SetPlayerTask
	{
		int playerid;
		int taskType;
		CVector position;
		float rotation;
		bool toggle;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				SetPlayerTask* packet = (SetPlayerTask*)data;
				packet->playerid = player->m_iPlayerId;
				CNetwork::SendPacketToAll(CPacketsID::SET_PLAYER_TASK, packet, sizeof(SetPlayerTask), ENET_PACKET_FLAG_RELIABLE, peer);
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				PedSay* packet = (PedSay*)data;

				if (packet->isPlayer)
				{
					packet->entityid = player->m_iPlayerId;
				}

				CNetwork::SendPacketToAll(CPacketsID::PED_SAY, packet, sizeof(PedSay), ENET_PACKET_FLAG_RELIABLE, peer);
			}
		}
	};

	struct AddProjectile
	{
		uint8_t creatorType;
		int creatorId;
		uint8_t projectileType; // eWeaponType
		CVector origin;
		float force;
		CVector dir;
		uint8_t targetType;
		int targetId;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				auto* packet = (AddProjectile*)data;
				packet->origin.x = std::clamp(packet->origin.x, -4000.0f, 4000.0f);
				packet->origin.y = std::clamp(packet->origin.y, -4000.0f, 4000.0f);
				packet->origin.z = std::clamp(packet->origin.z, -200.0f, 2000.0f);
				packet->force = std::clamp(packet->force, 0.0f, 500.0f);
				CNetwork::SendPacketToAll(CPacketsID::ADD_PROJECTILE, data, sizeof(AddProjectile), ENET_PACKET_FLAG_RELIABLE, peer);
				CFireSyncManager::OnProjectile(player, packet->origin, packet->projectileType);
			}
		}
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

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				CNetwork::SendPacketToAll(CPacketsID::TAG_UPDATE, data, sizeof(TagUpdate), ENET_PACKET_FLAG_RELIABLE, peer);
			}
		}
	};

	struct GangZoneStatePacket
	{
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t state;
		uint8_t currArea;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost)
			{
				return;
			}

			auto* packet = (GangZoneStatePacket*)data;
			packet->owner = std::min<uint8_t>(packet->owner, 10);
			packet->color = std::min<uint8_t>(packet->color, 255);
			packet->state = std::min<uint8_t>(packet->state, 4);
			player->m_nCurrArea = packet->currArea;

			GangZoneState cachedState{};
			cachedState.zoneId = packet->zoneId;
			cachedState.owner = packet->owner;
			cachedState.color = packet->color;
			cachedState.state = packet->state;
			cachedState.currArea = packet->currArea;
			ms_gangZoneStates[packet->zoneId] = cachedState;

			CNetwork::SendPacketToAll(CPacketsID::GANG_ZONE_STATE, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct GangGroupMembershipUpdatePacket
	{
		uint32_t sequence;
		int pedNetworkId;
		uint8_t gangGroupId;
		uint8_t action; // 0 add, 1 remove, 2 clear group

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost)
			{
				return;
			}

			auto* packet = (GangGroupMembershipUpdatePacket*)data;
			packet->gangGroupId = std::min<uint8_t>(packet->gangGroupId, 31);
			packet->action = std::min<uint8_t>(packet->action, 2);

			auto it = ms_gangGroupMembershipStates.find(packet->pedNetworkId);
			if (it != ms_gangGroupMembershipStates.end() && packet->sequence <= it->second.sequence)
			{
				return;
			}

			GangGroupMembershipState cachedState{};
			cachedState.sequence = packet->sequence;
			cachedState.pedNetworkId = packet->pedNetworkId;
			cachedState.gangGroupId = packet->gangGroupId;
			cachedState.action = packet->action;

			ms_gangGroupMembershipStates[packet->pedNetworkId] = cachedState;
			CNetwork::SendPacketToAll(CPacketsID::GANG_GROUP_MEMBERSHIP_UPDATE, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct GangRelationshipUpdatePacket
	{
		uint32_t sequence;
		uint8_t sourceGangGroupId;
		uint8_t targetGangGroupId;
		uint8_t relationshipFlags; // bit0 friendly, bit1 hostile

		static uint16_t GetRelationshipKey(uint8_t sourceGangGroupId, uint8_t targetGangGroupId)
		{
			return (uint16_t(sourceGangGroupId) << 8) | uint16_t(targetGangGroupId);
		}

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost)
			{
				return;
			}

			auto* packet = (GangRelationshipUpdatePacket*)data;
			packet->sourceGangGroupId = std::min<uint8_t>(packet->sourceGangGroupId, 31);
			packet->targetGangGroupId = std::min<uint8_t>(packet->targetGangGroupId, 31);
			packet->relationshipFlags &= 0x3;

			const uint16_t relationshipKey = GetRelationshipKey(packet->sourceGangGroupId, packet->targetGangGroupId);
			auto it = ms_gangRelationshipStates.find(relationshipKey);
			if (it != ms_gangRelationshipStates.end() && packet->sequence <= it->second.sequence)
			{
				return;
			}

			GangRelationshipState cachedState{};
			cachedState.sequence = packet->sequence;
			cachedState.sourceGangGroupId = packet->sourceGangGroupId;
			cachedState.targetGangGroupId = packet->targetGangGroupId;
			cachedState.relationshipFlags = packet->relationshipFlags;

			ms_gangRelationshipStates[relationshipKey] = cachedState;
			CNetwork::SendPacketToAll(CPacketsID::GANG_RELATIONSHIP_UPDATE, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct GangWarLifecycleEventPacket
	{
		uint32_t sequence;
		uint8_t eventType;
		uint8_t warState;
		uint8_t warPhase;
		uint8_t outcome;
		uint16_t zoneId;
		uint8_t owner;
		uint8_t color;
		uint8_t zoneState;
		uint8_t currArea;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			auto* player = CPlayerManager::GetPlayer(peer);
			if (!player || !player->m_bIsHost)
			{
				return;
			}

			auto* packet = (GangWarLifecycleEventPacket*)data;
			if (packet->eventType < 1 || packet->eventType > 4)
			{
				return;
			}

			if (packet->sequence <= ms_lastGangWarLifecycleState.sequence)
			{
				return;
			}

			packet->outcome = std::min<uint8_t>(packet->outcome, 2);
			packet->owner = std::min<uint8_t>(packet->owner, 10);
			packet->zoneState = std::min<uint8_t>(packet->zoneState, 4);
			player->m_nCurrArea = packet->currArea;

			GangWarLifecycleState cachedState{};
			cachedState.sequence = packet->sequence;
			cachedState.eventType = packet->eventType;
			cachedState.warState = packet->warState;
			cachedState.warPhase = packet->warPhase;
			cachedState.outcome = packet->outcome;
			cachedState.zoneId = packet->zoneId;
			cachedState.owner = packet->owner;
			cachedState.color = packet->color;
			cachedState.zoneState = packet->zoneState;
			cachedState.currArea = packet->currArea;
			ms_lastGangWarLifecycleState = cachedState;

			CNetwork::SendPacketToAll(CPacketsID::GANG_WAR_LIFECYCLE_EVENT, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);
		}
	};

	struct UpdateAllTags
	{
		TagUpdate tags[150];

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					CNetwork::SendPacketToAll(CPacketsID::UPDATE_ALL_TAGS, data, sizeof(UpdateAllTags), ENET_PACKET_FLAG_RELIABLE, peer);
				}
			}
		}
	};

	struct TeleportPlayerScripted
	{
		int playerid;
		CVector pos;
		float heading;

		static void Handle(ENetPeer* peer, void* data, int size)
		{
			if (auto player = CPlayerManager::GetPlayer(peer))
			{
				if (player->m_bIsHost)
				{
					TeleportPlayerScripted* packet = (TeleportPlayerScripted*)data;
					if (auto targetPlayer = CPlayerManager::GetPlayer(packet->playerid))
					{
						CNetwork::SendPacket(targetPlayer->m_pPeer, TELEPORT_PLAYER_SCRIPTED, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE);
					}
				}
			}
		}
	};
};
#endif
