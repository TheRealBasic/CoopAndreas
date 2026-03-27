
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstring>
#include <random>

#include "CPacketListener.h"
#include "CPacket.h"
#include "CNetwork.h"

#include "CPlayerManager.h"
#include "CVehicleManager.h"
#include "CPedManager.h"
#include "CPickupManager.h"
#include "CFireSyncManager.h"
#include "ConfigManager.h"

#include "semver.h"
#include "PlayerDisconnectReason.h"

std::unordered_map<unsigned short, std::unique_ptr<CPacketListener>> CNetwork::m_packetListeners;
ENetHost* CNetwork::m_pServer = nullptr;
bool CNetwork::m_bRunning = false;

namespace
{
    enum eHandshakeStage : uint8_t
    {
        HANDSHAKE_STAGE_SERVER_CHALLENGE = 0,
        HANDSHAKE_STAGE_CLIENT_RESPONSE = 1,
        HANDSHAKE_STAGE_SERVER_ACCEPTED = 2
    };

    struct PeerAuthState
    {
        bool authenticated = false;
        uint16_t clientProtocolVersion = 0;
        uint64_t clientCapabilities = 0;
        uint64_t negotiatedCapabilities = 0;
        uint32_t nonce = 0;
        std::chrono::steady_clock::time_point challengeSentAt = std::chrono::steady_clock::now();
    };

    struct PacketRateWindow
    {
        uint32_t count = 0;
        std::chrono::steady_clock::time_point windowStart = std::chrono::steady_clock::now();
    };

    static std::unordered_map<ENetPeer*, PeerAuthState> g_peerAuthStates;
    static std::unordered_map<ENetPeer*, std::unordered_map<uint16_t, PacketRateWindow>> g_peerRateWindows;

    static std::mt19937 g_nonceRng(std::random_device{}());

    uint32_t GenerateHandshakeNonce()
    {
        return std::uniform_int_distribution<uint32_t>(1, UINT32_MAX)(g_nonceRng);
    }

    uint16_t GetPacketRateLimitPerSecond(uint16_t packetId)
    {
        switch (packetId)
        {
            case CPacketsID::PLAYER_ONFOOT:
            case CPacketsID::VEHICLE_DRIVER_UPDATE:
            case CPacketsID::VEHICLE_PASSENGER_UPDATE:
            case CPacketsID::PED_ONFOOT:
            case CPacketsID::PED_DRIVER_UPDATE:
            case CPacketsID::PED_PASSENGER_UPDATE:
            case CPacketsID::PLAYER_AIM_SYNC:
            case CPacketsID::PLAYER_KEY_SYNC:
            case CPacketsID::MASS_PACKET_SEQUENCE:
                return 120;
            default:
                return 0;
        }
    }

    bool IsRateLimited(ENetPeer* peer, uint16_t packetId)
    {
        const uint16_t perSecondLimit = GetPacketRateLimitPerSecond(packetId);
        if (perSecondLimit == 0)
        {
            return false;
        }

        auto& peerWindows = g_peerRateWindows[peer];
        auto& window = peerWindows[packetId];
        const auto now = std::chrono::steady_clock::now();

        if (now - window.windowStart >= std::chrono::seconds(1))
        {
            window.windowStart = now;
            window.count = 0;
        }

        window.count++;
        return window.count > perSecondLimit;
    }

    const char* DisconnectReasonToString(uint32_t reason)
    {
        switch (reason)
        {
            case PLAYER_DISCONNECT_REASON_VERSION_MISMATCH:
                return "version_mismatch";
            case PLAYER_DISCONNECT_REASON_NAME_TAKEN:
                return "name_taken";
            case PLAYER_DISCONNECT_REASON_AUTH_TIMEOUT:
                return "auth_timeout";
            case PLAYER_DISCONNECT_REASON_AUTH_FAILED:
                return "auth_failed";
            case PLAYER_DISCONNECT_REASON_RATE_LIMIT:
                return "rate_limit";
            default:
                return "unknown";
        }
    }

    void LogListenerValidationReport()
    {
        std::unordered_set<unsigned short> registeredIds;
        registeredIds.reserve(CNetwork::m_packetListeners.size());
        for (const auto& [id, listener] : CNetwork::m_packetListeners)
        {
            registeredIds.insert(id);
        }

        const auto report = PacketRegistry::ValidateListeners(PacketRegistry::ReceiverRole::Server, registeredIds);
        if (report.ok)
        {
            printf("[Network] Packet registry validated (server listeners: %zu/%zu)\n",
                report.registeredCount,
                report.requiredCount);
        }
        else
        {
            printf("[WARN] Packet registry mismatch (missing=%zu unexpected=%zu)\n",
                report.missingCount,
                report.unexpectedCount);
            printf("[WARN] Details:\n%s", report.details.c_str());
        }
    }
}

bool CNetwork::Init(unsigned short port)
{
    if (CConfigManager::ms_pReader == nullptr)
    {
        printf("[ERROR] : ConfigManager must be initialized before CNetwork::Init\n");
        return false;
    }

    // init packet listeners
    CNetwork::InitListeners();

    if (enet_initialize() != 0) // try to init enet
    {
        printf("[ERROR] : ENET_INIT FAILED TO INITIALIZE\n");
        return false;
    }

    ENetAddress address;

    address.host = ENET_HOST_ANY; // bind server ip
    address.port = port; // bind server port

    constexpr size_t minPlayers = 1;
    constexpr size_t maxPlayers = MAX_SERVER_PLAYERS;

    const size_t configuredMaxPlayers = static_cast<size_t>(CConfigManager::GetConfigMaxPlayers());
    const size_t effectiveMaxPlayers = std::clamp(configuredMaxPlayers, minPlayers, maxPlayers);

    if (configuredMaxPlayers != effectiveMaxPlayers)
    {
        printf("[WARN] : Config maxplayers=%zu out of bounds, clamped to %zu (allowed range: %zu-%zu)\n",
            configuredMaxPlayers,
            effectiveMaxPlayers,
            minPlayers,
            maxPlayers);
    }

    m_pServer = enet_host_create(&address, effectiveMaxPlayers, 2, 0, 0); // create enet host

    if (m_pServer == NULL)
    {
        printf("[ERROR] : ENET_UDP_SERVER_SOCKET FAILED TO CREATE\n");
        m_packetListeners.clear();
        enet_deinitialize();
        return false;
    }

    printf("[!] : Server started on port %d (max players: %zu)\n", port, effectiveMaxPlayers);


    ENetEvent event;
    m_bRunning = true;
    while (m_bRunning) // waiting for event
    {
        CNetwork::ProcessAuthenticationTimeouts(m_pServer);
        CPickupManager::ProcessRespawns();
        CFireSyncManager::Process((uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        enet_host_service(m_pServer, &event, 1);
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                CNetwork::HandlePeerConnected(event);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                CNetwork::HandlePacketReceive(event);
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                CNetwork::HandlePlayerDisconnected(event);
                break;
            }
        }
    }

    CNetwork::Shutdown();
    return 0;
}

void CNetwork::InitListeners()
{
    m_packetListeners.clear();

    CNetwork::AddListener(CPacketsID::PLAYER_CONNECTED, CPlayerPackets::PlayerConnected::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_ONFOOT, CPlayerPackets::PlayerOnFoot::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_BULLET_SHOT, CPlayerPackets::PlayerBulletShot::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_PLACE_WAYPOINT, CPlayerPackets::PlayerPlaceWaypoint::Handle);
    CNetwork::AddListener(CPacketsID::ADD_EXPLOSION, CPlayerPackets::AddExplosion::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_SPAWN, CVehiclePackets::VehicleSpawn::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_REMOVE, CVehiclePackets::VehicleRemove::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_ENTER, CVehiclePackets::VehicleEnter::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_EXIT, CVehiclePackets::VehicleExit::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_DRIVER_UPDATE, CVehiclePackets::VehicleDriverUpdate::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_IDLE_UPDATE, CVehiclePackets::VehicleIdleUpdate::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_DAMAGE, CVehiclePackets::VehicleDamage::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_COMPONENT_ADD, CVehiclePackets::VehicleComponentAdd::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_COMPONENT_REMOVE, CVehiclePackets::VehicleComponentRemove::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_PASSENGER_UPDATE, CVehiclePackets::VehiclePassengerUpdate::Handle);
    CNetwork::AddListener(CPacketsID::VEHICLE_TRAILER_LINK_SYNC, CVehiclePackets::VehicleTrailerLinkSync::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_CHAT_MESSAGE, CPlayerPackets::PlayerChatMessage::Handle);
    CNetwork::AddListener(CPacketsID::PED_SPAWN, CPedPackets::PedSpawn::Handle);
    CNetwork::AddListener(CPacketsID::PED_REMOVE, CPedPackets::PedRemove::Handle);
    CNetwork::AddListener(CPacketsID::PED_ONFOOT, CPedPackets::PedOnFoot::Handle);
    CNetwork::AddListener(CPacketsID::GAME_WEATHER_TIME, CPlayerPackets::GameWeatherTime::Handle); // CPlayerPacket
    CNetwork::AddListener(CPacketsID::PLAYER_KEY_SYNC, CPlayerPackets::PlayerKeySync::Handle); 
    CNetwork::AddListener(CPacketsID::PED_ADD_TASK, CPedPackets::PedAddTask::Handle);
    CNetwork::AddListener(CPacketsID::PED_DRIVER_UPDATE, CPedPackets::PedDriverUpdate::Handle);
    CNetwork::AddListener(CPacketsID::PED_SHOT_SYNC, CPedPackets::PedShotSync::Handle);
    CNetwork::AddListener(CPacketsID::PED_PASSENGER_UPDATE, CPedPackets::PedPassengerSync::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_AIM_SYNC, CPlayerPackets::PlayerAimSync::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_WANTED_LEVEL, CPlayerPackets::PlayerWantedLevel::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_STATS, CPlayerPackets::PlayerStats::Handle);
    CNetwork::AddListener(CPacketsID::REBUILD_PLAYER, CPlayerPackets::RebuildPlayer::Handle);
    CNetwork::AddListener(CPacketsID::RESPAWN_PLAYER, CPlayerPackets::RespawnPlayer::Handle);
    CNetwork::AddListener(CPacketsID::START_CUTSCENE, CPlayerPackets::StartCutscene::Handle);
    CNetwork::AddListener(CPacketsID::SKIP_CUTSCENE, CPlayerPackets::SkipCutscene::Handle);
    CNetwork::AddListener(CPacketsID::OPCODE_SYNC, CPlayerPackets::OpCodeSync::Handle);
    CNetwork::AddListener(CPacketsID::ON_MISSION_FLAG_SYNC, CPlayerPackets::OnMissionFlagSync::Handle);
    CNetwork::AddListener(CPacketsID::MISSION_FLOW_SYNC, CPlayerPackets::MissionFlowSync::Handle);
    CNetwork::AddListener(CPacketsID::UPDATE_ENTITY_BLIP, CPlayerPackets::UpdateEntityBlip::Handle);
    CNetwork::AddListener(CPacketsID::REMOVE_ENTITY_BLIP, CPlayerPackets::RemoveEntityBlip::Handle);
    CNetwork::AddListener(CPacketsID::ADD_MESSAGE_GXT, CPlayerPackets::AddMessageGXT::Handle);
    CNetwork::AddListener(CPacketsID::REMOVE_MESSAGE_GXT, CPlayerPackets::RemoveMessageGXT::Handle);
    CNetwork::AddListener(CPacketsID::CLEAR_ENTITY_BLIPS, CPlayerPackets::ClearEntityBlips::Handle);
    CNetwork::AddListener(CPacketsID::PLAY_MISSION_AUDIO, CPlayerPackets::PlayMissionAudio::Handle);
    CNetwork::AddListener(CPacketsID::UPDATE_CHECKPOINT, CPlayerPackets::UpdateCheckpoint::Handle);
    CNetwork::AddListener(CPacketsID::REMOVE_CHECKPOINT, CPlayerPackets::RemoveCheckpoint::Handle);
    CNetwork::AddListener(CPacketsID::ENEX_SYNC, CPlayerPackets::EnExSync::Handle);
    CNetwork::AddListener(CPacketsID::CREATE_STATIC_BLIP, CPlayerPackets::CreateStaticBlip::Handle);
    CNetwork::AddListener(CPacketsID::SET_VEHICLE_CREATED_BY, CVehiclePackets::SetVehicleCreatedBy::Handle);
    CNetwork::AddListener(CPacketsID::SET_PLAYER_TASK, CPlayerPackets::SetPlayerTask::Handle);
    CNetwork::AddListener(CPacketsID::PED_SAY, CPlayerPackets::PedSay::Handle);
    CNetwork::AddListener(CPacketsID::PED_CLAIM_ON_RELEASE, CPedPackets::PedClaimOnRelease::Handle);
    CNetwork::AddListener(CPacketsID::PED_CANCEL_CLAIM, CPedPackets::PedCancelClaim::Handle);
    CNetwork::AddListener(CPacketsID::PED_RESET_ALL_CLAIMS, CPedPackets::PedResetAllClaims::Handle);
    CNetwork::AddListener(CPacketsID::PED_TAKE_HOST, CPedPackets::PedTakeHost::Handle);
    CNetwork::AddListener(CPacketsID::PERFORM_TASK_SEQUENCE, CPedPackets::PerformTaskSequence::Handle);
    CNetwork::AddListener(CPacketsID::ADD_PROJECTILE, CPlayerPackets::AddProjectile::Handle);
    CNetwork::AddListener(CPacketsID::TAG_UPDATE, CPlayerPackets::TagUpdate::Handle);
    CNetwork::AddListener(CPacketsID::UPDATE_ALL_TAGS, CPlayerPackets::UpdateAllTags::Handle);
    CNetwork::AddListener(CPacketsID::GANG_ZONE_STATE, CPlayerPackets::GangZoneStatePacket::Handle);
    CNetwork::AddListener(CPacketsID::GANG_GROUP_MEMBERSHIP_UPDATE, CPlayerPackets::GangGroupMembershipUpdatePacket::Handle);
    CNetwork::AddListener(CPacketsID::GANG_RELATIONSHIP_UPDATE, CPlayerPackets::GangRelationshipUpdatePacket::Handle);
    CNetwork::AddListener(CPacketsID::GANG_WAR_LIFECYCLE_EVENT, CPlayerPackets::GangWarLifecycleEventPacket::Handle);
    CNetwork::AddListener(CPacketsID::TELEPORT_PLAYER_SCRIPTED, CPlayerPackets::TeleportPlayerScripted::Handle);
    CNetwork::AddListener(CPacketsID::PICKUP_SNAPSHOT_REQUEST, CPlayerPackets::PickupSnapshotRequest::Handle);
    CNetwork::AddListener(CPacketsID::PICKUP_COLLECT_REQUEST, CPickupPackets::PickupCollectRequest::Handle);
    CNetwork::AddListener(CPacketsID::PROPERTY_STATE_DELTA, CPlayerPackets::PropertyStateDelta::Handle);
    CNetwork::AddListener(CPacketsID::SUBMISSION_MISSION_STATE_DELTA, CPlayerPackets::SubmissionMissionStateDelta::Handle);
    CNetwork::AddListener(CPacketsID::PLAYER_JETPACK_TRANSITION, CPlayerPackets::PlayerJetpackTransition::Handle);
    CNetwork::AddListener(CPacketsID::CHEAT_EFFECT_TRIGGER, CPlayerPackets::CheatEffectTriggerPacket::Handle);

    LogListenerValidationReport();
}

void CNetwork::SendPacket(ENetPeer* peer, unsigned short id, void* data, size_t dataSize, ENetPacketFlag flag)
{
    size_t packetSize = sizeof(uint16_t) + dataSize;
    std::vector<uint8_t> packetData(packetSize);
    memcpy(packetData.data(), &id, sizeof(uint16_t));
    memcpy(packetData.data() + sizeof(uint16_t), data, dataSize);
    ENetPacket* packet = enet_packet_create(packetData.data(), packetSize, flag);

    enet_peer_send(peer, 0, packet);
}

void CNetwork::SendPacketToAll(unsigned short id, void* data, size_t dataSize, ENetPacketFlag flag, ENetPeer* dontShareWith)
{
    size_t packetSize = sizeof(uint16_t) + dataSize;
    std::vector<uint8_t> packetData(packetSize);
    memcpy(packetData.data(), &id, sizeof(uint16_t));
    memcpy(packetData.data() + sizeof(uint16_t), data, dataSize);
    ENetPacket* packet = enet_packet_create(packetData.data(), packetSize, flag);

    for (int i = 0; i != CPlayerManager::m_pPlayers.size(); i++)
    {
        if (CPlayerManager::m_pPlayers[i]->m_pPeer != dontShareWith)
        {
            enet_peer_send(CPlayerManager::m_pPlayers[i]->m_pPeer, 0, packet);
        }
    }
}

void CNetwork::SendPacketRawToAll(void* data, size_t dataSize, ENetPacketFlag flag, ENetPeer* dontShareWith)
{
    ENetPacket* packet = enet_packet_create(data, dataSize, flag);

    for (int i = 0; i != CPlayerManager::m_pPlayers.size(); i++)
    {
        if (CPlayerManager::m_pPlayers[i]->m_pPeer != dontShareWith)
        {
            enet_peer_send(CPlayerManager::m_pPlayers[i]->m_pPeer, 0, packet);
        }
    }
}

void CNetwork::HandlePeerConnected(ENetEvent& event)
{
    printf("[Game] : A new client connected from %i.%i.%i.%i:%u.\n", 
        event.peer->address.host & 0xFF, 
        (event.peer->address.host >> 8) & 0xFF, 
        (event.peer->address.host >> 16) & 0xFF, 
        (event.peer->address.host >> 24) & 0xFF,
        event.peer->address.port);

    // set player disconnection timeout
    enet_peer_timeout(event.peer, 10000, 6000, 10000); //timeoutLimit, timeoutMinimum, timeoutMaximum

    PeerAuthState authState{};
    authState.authenticated = !CConfigManager::GetAuthStrictMode();
    authState.challengeSentAt = std::chrono::steady_clock::now();

    if (!authState.authenticated)
    {
        authState.nonce = GenerateHandshakeNonce();

        CPlayerPackets::PlayerHandshake challenge{};
        challenge.stage = HANDSHAKE_STAGE_SERVER_CHALLENGE;
        challenge.protocolVersion = PacketRegistry::PROTOCOL_VERSION;
        challenge.capabilityBitmap = PacketRegistry::PROTOCOL_CAPABILITIES;
        challenge.nonce = authState.nonce;
        CNetwork::SendPacket(event.peer, CPacketsID::PLAYER_HANDSHAKE, &challenge, sizeof(challenge), ENET_PACKET_FLAG_RELIABLE);
    }

    g_peerAuthStates[event.peer] = authState;
}

void CNetwork::HandlePlayerDisconnected(ENetEvent& event)
{
    const uint32_t disconnectReason = event.data;
    printf("[Network] : Peer %i.%i.%i.%i:%u disconnected (reason=%s/%u)\n",
        event.peer->address.host & 0xFF,
        (event.peer->address.host >> 8) & 0xFF,
        (event.peer->address.host >> 16) & 0xFF,
        (event.peer->address.host >> 24) & 0xFF,
        event.peer->address.port,
        DisconnectReasonToString(disconnectReason),
        disconnectReason);

    g_peerAuthStates.erase(event.peer);
    g_peerRateWindows.erase(event.peer);

    // disconnect player

    // find player instance by enetpeer
    CPlayer* player = CPlayerManager::GetPlayer(event.peer);
    
    if (player == nullptr)
    {
        return;
    }

    CVehicle* vehicle = CVehicleManager::GetVehicle(player->m_nVehicleId);

    if (vehicle != nullptr)
    {
        vehicle->m_pPlayers[player->m_nSeatId] = nullptr;
    }
    
    if (CPlayerPackets::EnExSync::ms_pLastPlayerOwner == player)
    {
        CPlayerPackets::EnExSync::ms_pLastPlayerOwner = nullptr;
    }

    CPedManager::RemoveAllHostedAndNotify(player);
    CVehicleManager::RemoveAllHostedAndNotify(player);
    CPlayerPackets::OnPlayerDisconnectedFromCutsceneVote(player->m_iPlayerId);
    CFireSyncManager::OnPlayerDisconnected(player->m_iPlayerId);
    CPlayerPackets::ResetCheatEffectThrottle(event.peer);

    // remove
    CPlayerManager::Remove(player);

    // create PlayerDisconnected packet struct
    CPlayerPackets::PlayerDisconnected packet =
    {
        player->m_iPlayerId // id
    };

    // send to all
    CNetwork::SendPacketToAll(CPacketsID::PLAYER_DISCONNECTED, &packet, sizeof (CPlayerPackets::PlayerDisconnected), (ENetPacketFlag)0, event.peer);

    printf("[Game] : %i Disconnected.\n", player->m_iPlayerId);

    CPlayerManager::AssignHostToFirstPlayer();
}

void CNetwork::HandlePacketReceive(ENetEvent& event)
{
    uint16_t packetid;
    memcpy(&packetid, event.packet->data, 2);
    if (packetid >= PACKET_ID_MAX)
    {
        return;
    }

    const auto& contract = PacketRegistry::Contracts()[packetid];
    const size_t payloadSize = event.packet->dataLength >= sizeof(uint16_t) ? (event.packet->dataLength - sizeof(uint16_t)) : 0;
    if (payloadSize < contract.minPayloadSize)
    {
        printf("[Network] Dropping %s payload too small (%zu < %u)\n",
            contract.name,
            payloadSize,
            contract.minPayloadSize);
        return;
    }

    auto authIt = g_peerAuthStates.find(event.peer);
    if (authIt == g_peerAuthStates.end())
    {
        return;
    }

    PeerAuthState& authState = authIt->second;

    if (!authState.authenticated)
    {
        if (packetid != CPacketsID::PLAYER_HANDSHAKE)
        {
            return;
        }

        if (event.packet->dataLength < (2 + sizeof(CPlayerPackets::PlayerHandshake)))
        {
            return;
        }

        auto* handshakePacket = reinterpret_cast<CPlayerPackets::PlayerHandshake*>(event.packet->data + 2);
        if (handshakePacket->stage != HANDSHAKE_STAGE_CLIENT_RESPONSE || handshakePacket->nonce != authState.nonce)
        {
            CPlayerPackets::PlayerDisconnected disconnectPacket{};
            disconnectPacket.id = -1;
            disconnectPacket.reason = PLAYER_DISCONNECT_REASON_AUTH_FAILED;
            CNetwork::SendPacket(event.peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_disconnect_later(event.peer, PLAYER_DISCONNECT_REASON_AUTH_FAILED);
            return;
        }

        if (handshakePacket->responseHash != CNetwork::ComputeHandshakeResponse(authState.nonce))
        {
            CPlayerPackets::PlayerDisconnected disconnectPacket{};
            disconnectPacket.id = -1;
            disconnectPacket.reason = PLAYER_DISCONNECT_REASON_AUTH_FAILED;
            CNetwork::SendPacket(event.peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_disconnect_later(event.peer, PLAYER_DISCONNECT_REASON_AUTH_FAILED);
            return;
        }

        if (handshakePacket->protocolVersion == 0 || (handshakePacket->capabilityBitmap & PacketRegistry::CAP_BASELINE) == 0)
        {
            CPlayerPackets::PlayerDisconnected disconnectPacket{};
            disconnectPacket.id = -1;
            disconnectPacket.reason = PLAYER_DISCONNECT_REASON_AUTH_FAILED;
            CNetwork::SendPacket(event.peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_disconnect_later(event.peer, PLAYER_DISCONNECT_REASON_AUTH_FAILED);
            return;
        }

        authState.clientProtocolVersion = handshakePacket->protocolVersion;
        authState.clientCapabilities = handshakePacket->capabilityBitmap;
        authState.negotiatedCapabilities = handshakePacket->capabilityBitmap & PacketRegistry::PROTOCOL_CAPABILITIES;

        authState.authenticated = true;
        return;
    }

    if (IsRateLimited(event.peer, packetid))
    {
        CPlayerPackets::PlayerDisconnected disconnectPacket{};
        disconnectPacket.id = -1;
        disconnectPacket.reason = PLAYER_DISCONNECT_REASON_RATE_LIMIT;
        CNetwork::SendPacket(event.peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_disconnect_later(event.peer, PLAYER_DISCONNECT_REASON_RATE_LIMIT);
        return;
    }

    if (packetid == CPacketsID::MASS_PACKET_SEQUENCE)
    {
        CNetwork::SendPacketRawToAll(event.packet->data, event.packet->dataLength, (ENetPacketFlag)event.packet->flags, event.peer);
    }
    else
    {
        auto it = m_packetListeners.find(packetid);
        if (it != m_packetListeners.end())
        {
            uint8_t* pData = event.packet->data + 2;
            int32_t nLength = event.packet->dataLength - 2;
            it->second->m_callback(event.peer, pData, nLength);
        }
    }
}

void CNetwork::AddListener(unsigned short id, void(*callback)(ENetPeer*, void*, int))
{
    m_packetListeners[id] = std::make_unique<CPacketListener>(id, callback);
}

void CNetwork::Shutdown()
{
    m_bRunning = false;
    m_packetListeners.clear();
    g_peerAuthStates.clear();
    g_peerRateWindows.clear();

    if (m_pServer != nullptr)
    {
        enet_host_destroy(m_pServer);
        m_pServer = nullptr;
    }

    enet_deinitialize();
    printf("[!] : Server Shutdown (ENET_DEINITIALIZE)\n");
}

uint32_t CNetwork::ComputeHandshakeResponse(uint32_t nonce)
{
    constexpr uint32_t kSeed = 0x811C9DC5u;
    constexpr uint32_t kPrime = 0x01000193u;
    uint32_t hash = kSeed;
    const uint32_t packedVersion = semver_parse(COOPANDREAS_VERSION, nullptr);

    auto mix = [&hash, kPrime](uint8_t value)
    {
        hash ^= value;
        hash *= kPrime;
    };

    for (size_t i = 0; i < sizeof(nonce); ++i)
    {
        mix(static_cast<uint8_t>((nonce >> (i * 8)) & 0xFF));
    }

    for (size_t i = 0; i < sizeof(packedVersion); ++i)
    {
        mix(static_cast<uint8_t>((packedVersion >> (i * 8)) & 0xFF));
    }

    return hash;
}

void CNetwork::ProcessAuthenticationTimeouts(ENetHost* server)
{
    if (!CConfigManager::GetAuthStrictMode())
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(CConfigManager::GetAuthTimeoutMs());

    for (size_t i = 0; i < server->peerCount; ++i)
    {
        ENetPeer* peer = &server->peers[i];
        if (peer->state != ENET_PEER_STATE_CONNECTED)
        {
            continue;
        }

        auto authIt = g_peerAuthStates.find(peer);
        if (authIt == g_peerAuthStates.end() || authIt->second.authenticated)
        {
            continue;
        }

        if (now - authIt->second.challengeSentAt >= timeout)
        {
            CPlayerPackets::PlayerDisconnected disconnectPacket{};
            disconnectPacket.id = -1;
            disconnectPacket.reason = PLAYER_DISCONNECT_REASON_AUTH_TIMEOUT;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_disconnect_later(peer, PLAYER_DISCONNECT_REASON_AUTH_TIMEOUT);
        }
    }
}

void CNetwork::HandlePlayerConnected(ENetPeer* peer, void* data, int size)
{
    CPlayerPackets::PlayerConnected* packet = (CPlayerPackets::PlayerConnected*)data;

#ifndef _DEBUG
    for (auto player : CPlayerManager::m_pPlayers)
    {
        if (strncmp(player->m_Name, packet->name, sizeof(player->m_Name)) == 0)
        {
            CPlayerPackets::PlayerDisconnected disconnectPacket{};
            disconnectPacket.id = -1;
            disconnectPacket.reason = PLAYER_DISCONNECT_REASON_NAME_TAKEN;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);
            return;
        }
    }
#endif

    int freeId = CPlayerManager::GetFreeID();
    CPlayer* player = new CPlayer(peer, freeId);
    std::strncpy(player->m_Name, packet->name, sizeof(player->m_Name) - 1);
    player->m_Name[sizeof(player->m_Name) - 1] = '\0';
    CPlayerManager::Add(player);

    uint32_t packedVersion = semver_parse(COOPANDREAS_VERSION, nullptr);
    char buffer[23];
    semver_t playerVersion;
    semver_unpack(packet->version, &playerVersion);
    semver_to_string(&playerVersion, buffer, sizeof(buffer));
    buffer[22] = '\0';

    if (packedVersion != packet->version)
    {
        CPlayerPackets::PlayerDisconnected disconnectPacket{};
        disconnectPacket.id = -1;
        disconnectPacket.reason = PLAYER_DISCONNECT_REASON_VERSION_MISMATCH;
        disconnectPacket.version = packedVersion;
        CNetwork::SendPacket(peer, CPacketsID::PLAYER_DISCONNECTED, &disconnectPacket, sizeof(disconnectPacket), ENET_PACKET_FLAG_RELIABLE);

        enet_peer_disconnect_later(peer, packedVersion);
        return;
    }

    printf("freeId %d name %s version %s\n", freeId, packet->name, buffer);
    
    packet->id = freeId;

    CNetwork::SendPacketToAll(CPacketsID::PLAYER_CONNECTED, packet, sizeof(*packet), ENET_PACKET_FLAG_RELIABLE, peer);

    CPlayerPackets::SendPickupBootstrap(peer);

    for (auto i : CPlayerManager::m_pPlayers)
    {
        if (i->m_iPlayerId == freeId)
            continue;

        CPlayerPackets::PlayerConnected newPlayerPacket{};
        newPlayerPacket.id = i->m_iPlayerId;
        newPlayerPacket.isAlreadyConnected = true;
        std::strncpy(newPlayerPacket.name, i->m_Name, sizeof(newPlayerPacket.name) - 1);
        newPlayerPacket.name[sizeof(newPlayerPacket.name) - 1] = '\0';

        CNetwork::SendPacket(peer, CPacketsID::PLAYER_CONNECTED, &newPlayerPacket, sizeof(CPlayerPackets::PlayerConnected), ENET_PACKET_FLAG_RELIABLE);

        if (i->m_ucSyncFlags.bStatsModified)
        {
            CPlayerPackets::PlayerStats statsPacket{};
            statsPacket.playerid = i->m_iPlayerId;
            memcpy(statsPacket.stats, i->m_afStats, sizeof(i->m_afStats));
            statsPacket.money = i->m_nMoney;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_STATS, &statsPacket, sizeof(statsPacket), ENET_PACKET_FLAG_RELIABLE);
        }

        if (i->m_ucSyncFlags.bWantedLevelModified)
        {
            CPlayerPackets::PlayerWantedLevel wantedPacket{};
            wantedPacket.playerid = i->m_iPlayerId;
            wantedPacket.wantedLevel = i->m_nWantedLevel;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_WANTED_LEVEL, &wantedPacket, sizeof(wantedPacket), ENET_PACKET_FLAG_RELIABLE);
        }

        if (i->m_ucSyncFlags.bClothesModified)
        {
            CPlayerPackets::RebuildPlayer rebuildPacket{};
            rebuildPacket.playerid = i->m_iPlayerId;
            memcpy(rebuildPacket.m_anModelKeys, i->m_anModelKeys, sizeof(i->m_anModelKeys));
            memcpy(rebuildPacket.m_anTextureKeys, i->m_anTextureKeys, sizeof(i->m_anTextureKeys));
            rebuildPacket.m_fFatStat = i->m_fFatStat;
            rebuildPacket.m_fMuscleStat = i->m_fMuscleStat;
            CNetwork::SendPacket(peer, CPacketsID::REBUILD_PLAYER, &rebuildPacket, sizeof(rebuildPacket), ENET_PACKET_FLAG_RELIABLE);
        }

        if (i->m_bHasJetpack)
        {
            CPlayerPackets::PlayerJetpackTransition jetpackPacket{};
            jetpackPacket.playerid = i->m_iPlayerId;
            jetpackPacket.intent = CPlayerPackets::JETPACK_TRANSITION_ACQUIRE;
            jetpackPacket.hasJetpack = true;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_JETPACK_TRANSITION, &jetpackPacket, sizeof(jetpackPacket), ENET_PACKET_FLAG_RELIABLE);
            printf("[JetpackTransition][Server][Snapshot] player=%d hasJetpack=1\n", i->m_iPlayerId);
        }

        if (i->m_nSniperAimTick != 0)
        {
            CPlayerPackets::PlayerSniperAimMarkerState sniperAimPacket{};
            sniperAimPacket.playerid = i->m_iPlayerId;
            sniperAimPacket.source = i->m_vecSniperAimSource;
            sniperAimPacket.direction = i->m_vecSniperAimDirection;
            sniperAimPacket.range = i->m_fSniperAimRange;
            sniperAimPacket.visible = i->m_bSniperAimVisible;
            sniperAimPacket.tick = i->m_nSniperAimTick;
            CNetwork::SendPacket(peer, CPacketsID::PLAYER_SNIPER_AIM_MARKER_STATE, &sniperAimPacket, sizeof(sniperAimPacket), ENET_PACKET_FLAG_RELIABLE);
        }
    }

    for (auto i : CVehicleManager::m_pVehicles)
    {
        CVehiclePackets::VehicleSpawn vehicleSpawnPacket{};
        vehicleSpawnPacket.vehicleid = i->m_nVehicleId;
        vehicleSpawnPacket.modelid = i->m_nModelId;
        vehicleSpawnPacket.pos = i->m_vecPosition;
        vehicleSpawnPacket.rot = static_cast<float>(i->m_vecRotation.z * (3.141592 / 180)); // convert to radians
        vehicleSpawnPacket.color1 = i->m_nPrimaryColor;
        vehicleSpawnPacket.color2 = i->m_nSecondaryColor;

        CNetwork::SendPacket(peer, CPacketsID::VEHICLE_SPAWN, &vehicleSpawnPacket, sizeof vehicleSpawnPacket, ENET_PACKET_FLAG_RELIABLE);

        bool modifiedDamageStatus = false;

        for (unsigned char j = 0; j < 23; j++)
        {
            if (i->m_damageManager_padding[j])
            {
                modifiedDamageStatus = true;
                break;
            }
        }

        if (modifiedDamageStatus)
        {
            CVehiclePackets::VehicleDamage vehicleDamagePacket{};
            vehicleDamagePacket.vehicleid = i->m_nVehicleId;
            memcpy(&vehicleDamagePacket.damageManager_padding, i->m_damageManager_padding, 23);
            CNetwork::SendPacket(peer, CPacketsID::VEHICLE_DAMAGE, &vehicleDamagePacket, sizeof vehicleDamagePacket, ENET_PACKET_FLAG_RELIABLE);
        }

        for (int component : i->m_pComponents)
        {
            CVehiclePackets::VehicleComponentAdd vehicleComponentAddPacket{};
            vehicleComponentAddPacket.vehicleid = i->m_nVehicleId;
            vehicleComponentAddPacket.componentid = component;
            CNetwork::SendPacket(peer, CPacketsID::VEHICLE_COMPONENT_ADD, &vehicleComponentAddPacket, sizeof vehicleComponentAddPacket, ENET_PACKET_FLAG_RELIABLE);
        }
    }

    for (auto i : CPedManager::m_pPeds)
    {
        CPedPackets::PedSpawn packet{};
        packet.pedid = i->m_nPedId;
        packet.modelId = i->m_nModelId;
        packet.pos = i->m_vecPos;
        packet.pedType = i->m_nPedType;
        packet.createdBy = i->m_nCreatedBy;
        std::strncpy(packet.specialModelName, i->m_szSpecialModelName, sizeof(packet.specialModelName) - 1);
        packet.specialModelName[sizeof(packet.specialModelName) - 1] = '\0';
        CNetwork::SendPacket(peer, CPacketsID::PED_SPAWN, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);
    }

    if (CPlayerPackets::EnExSync::ms_pLastPlayerOwner)
    {
        if (std::find(CPlayerManager::m_pPlayers.begin(), CPlayerManager::m_pPlayers.end(), CPlayerPackets::EnExSync::ms_pLastPlayerOwner)
            != CPlayerManager::m_pPlayers.end())
        {
            CNetwork::SendPacket(peer, CPacketsID::ENEX_SYNC, CPlayerPackets::EnExSync::ms_vLastData.data(), CPlayerPackets::EnExSync::ms_vLastData.size(), ENET_PACKET_FLAG_RELIABLE);
        }
    }

    CPlayerPackets::SendMapStateSnapshot(peer);
    CPlayerPackets::SendMissionStateSnapshot(peer);
    CPlayerPackets::SendPropertyStateSnapshot(peer);
    CPlayerPackets::SendSubmissionMissionStateSnapshot(peer);

    CFireSyncManager::SendSnapshotTo(peer, player->m_vecPosition);

    CPlayerPackets::PlayerHandshake handshakePacket{};
    handshakePacket.stage = HANDSHAKE_STAGE_SERVER_ACCEPTED;
    auto authIt = g_peerAuthStates.find(peer);
    if (authIt != g_peerAuthStates.end())
    {
        handshakePacket.protocolVersion = std::min<uint16_t>(PacketRegistry::PROTOCOL_VERSION, authIt->second.clientProtocolVersion);
        handshakePacket.capabilityBitmap = authIt->second.negotiatedCapabilities;
        printf("[Network] Handshake accepted peer=%i protocol=%u negotiatedCaps=0x%llx clientCaps=0x%llx\n",
            freeId,
            handshakePacket.protocolVersion,
            static_cast<unsigned long long>(handshakePacket.capabilityBitmap),
            static_cast<unsigned long long>(authIt->second.clientCapabilities));
    }
    else
    {
        handshakePacket.protocolVersion = PacketRegistry::PROTOCOL_VERSION;
        handshakePacket.capabilityBitmap = PacketRegistry::PROTOCOL_CAPABILITIES;
    }
    handshakePacket.yourid = freeId;
    CNetwork::SendPacket(peer, CPacketsID::PLAYER_HANDSHAKE, &handshakePacket, sizeof handshakePacket, ENET_PACKET_FLAG_RELIABLE);

    CPlayerManager::AssignHostToFirstPlayer();
}
