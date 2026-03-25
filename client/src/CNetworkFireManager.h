#pragma once

#include "CPackets.h"
#include <unordered_map>

class CNetworkFireManager
{
public:
    static void HandleCreate(const CPackets::FireCreate& packet);
    static void HandleUpdate(const CPackets::FireUpdate& packet);
    static void HandleRemove(const CPackets::FireRemove& packet);
    static void Reset();

private:
    struct FireEntity
    {
        int scriptHandle = -1;
        CPackets::FireCreate state{};
    };

    static int StartScriptFire(const CPackets::FireCreate& packet);
    static void RemoveScriptFireSafe(int scriptHandle);

    static std::unordered_map<uint32_t, FireEntity> ms_fires;
};
