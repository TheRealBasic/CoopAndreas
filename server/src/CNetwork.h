#pragma once

#ifndef _CNETWORK_H_
	#define _CNETWORK_H_ 

#define COOPANDREAS_VERSION "0.2.2-alpha"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "enet/enet.h"

#include "CPacketListener.h"

#define MAX_SERVER_PLAYERS (8)

class CNetwork
{
	public:
		CNetwork();
		
		static std::unordered_map<unsigned short, std::unique_ptr<CPacketListener>> m_packetListeners;
		static ENetHost* m_pServer;
		static bool m_bRunning;
		static bool Init(unsigned short port);
		static void InitListeners();
		static void SendPacket(ENetPeer* peer, unsigned short id, void* data, size_t dataSize, ENetPacketFlag flag = (ENetPacketFlag)0);
		static void SendPacketToAll(unsigned short id, void* data, size_t dataSize, ENetPacketFlag flag = (ENetPacketFlag)0, ENetPeer* dontShareWith = nullptr);
		static void SendPacketRawToAll(void* data, size_t dataSize, ENetPacketFlag flag = (ENetPacketFlag)0, ENetPeer* dontShareWith = nullptr);
		static void HandlePlayerConnected(ENetPeer* peer, void* data, int size);
		~CNetwork();
	private:
		static uint32_t ComputeHandshakeResponse(uint32_t nonce);
		static void HandlePeerConnected(ENetEvent& event);
		static void HandlePlayerDisconnected(ENetEvent& event);
		static void HandlePacketReceive(ENetEvent& event);
		static void ProcessAuthenticationTimeouts(ENetHost* server);
		static void Shutdown();
		static void AddListener(unsigned short id, void(*callback)(ENetPeer*, void*, int));

	
};

#endif
