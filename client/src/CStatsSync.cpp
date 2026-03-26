#include "stdafx.h"

int m_anStoredIntStats[224];
float m_afStoredFloatStats[83];

constexpr int MAX_INT_STATS = sizeof(m_anStoredIntStats) / sizeof(int);
constexpr int MAX_FLOAT_STATS = sizeof(m_afStoredFloatStats) / sizeof(float);
static_assert(CStatsSync::SYNCED_STATS_COUNT == CPackets::PLAYER_STATS_SYNCED_COUNT, "Stats sync count must match packet payload capacity");

std::array<eStats, CStatsSync::SYNCED_STATS_COUNT> CStatsSync::m_aeSyncedStats =
{
    STAT_PISTOL_SKILL,
    STAT_SILENCED_PISTOL_SKILL,
    STAT_DESERT_EAGLE_SKILL,
    STAT_SHOTGUN_SKILL,
    STAT_SAWN_OFF_SHOTGUN_SKILL,
    STAT_COMBAT_SHOTGUN_SKILL,
    STAT_MACHINE_PISTOL_SKILL,
    STAT_SMG_SKILL,
    STAT_AK_47_SKILL,
    STAT_M4_SKILL,
    STAT_RIFLE_SKILL,
    STAT_MAX_HEALTH,
    STAT_STAMINA,
    STAT_LUNG_CAPACITY
};

void CStatsSync::ApplyNetworkPlayerContext(CNetworkPlayer* player)
{
    m_nRemoteContextDepth++;

    for (int i = 0; i < MAX_INT_STATS; ++i)
        m_anStoredIntStats[i] = CStats::StatTypesInt[i];

    for (int i = 0; i < MAX_FLOAT_STATS; ++i)
        m_afStoredFloatStats[i] = CStats::StatTypesFloat[i];

    for (int i = 0; i < MAX_INT_STATS; ++i)
        CStats::StatTypesInt[i] = player->m_stats.m_aStatsInt[i];

    for (int i = 0; i < MAX_FLOAT_STATS; ++i)
        CStats::StatTypesFloat[i] = player->m_stats.m_aStatsFloat[i];
}

void CStatsSync::ApplyLocalContext()
{
    for (int i = 0; i < MAX_INT_STATS; ++i)
        CStats::StatTypesInt[i] = m_anStoredIntStats[i];

    for (int i = 0; i < MAX_FLOAT_STATS; ++i)
        CStats::StatTypesFloat[i] = m_afStoredFloatStats[i];

    if (m_nRemoteContextDepth > 0)
    {
        m_nRemoteContextDepth--;
    }
}


void CStatsSync::NotifyChanged()
{
    if (m_nRemoteContextDepth > 0)
    {
        return;
    }

    CPackets::PlayerStats packet{};
    
    for (uint8_t i = 0; i < CStatsSync::SYNCED_STATS_COUNT; i++)
    {
        packet.stats[i] = CStats::GetStatValue(m_aeSyncedStats[i]);
    }

    if (auto localPlayer = FindPlayerPed(0))
    {
        if (auto playerInfo = localPlayer->GetPlayerInfoForThisPlayerPed())
        {
            packet.money = std::clamp(playerInfo->m_nMoney, 0, INT_MAX);
        }
    }

    CNetwork::SendPacket(CPacketsID::PLAYER_STATS, &packet, sizeof packet, ENET_PACKET_FLAG_RELIABLE);
}

void CStatsSync::NotifyWantedLevelChanged()
{
    if (m_nRemoteContextDepth > 0)
    {
        return;
    }

    if (!CNetwork::m_bConnected)
    {
        return;
    }

    auto* localPlayer = FindPlayerPed(0);
    if (!localPlayer)
    {
        return;
    }

    const int wantedLevel = std::clamp(localPlayer->GetWantedLevel(), 0, 6);
    if (wantedLevel == m_nLastWantedLevelSent)
    {
        return;
    }

    CPackets::PlayerWantedLevel packet{};
    packet.wantedLevel = static_cast<uint8_t>(wantedLevel);
    CNetwork::SendPacket(CPacketsID::PLAYER_WANTED_LEVEL, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    m_nLastWantedLevelSent = wantedLevel;
}

void CStatsSync::TriggerWantedLevelReset()
{
    if (m_nRemoteContextDepth > 0)
    {
        return;
    }

    if (!CNetwork::m_bConnected || m_nLastWantedLevelSent == 0)
    {
        return;
    }

    CPackets::PlayerWantedLevel packet{};
    packet.wantedLevel = 0;
    CNetwork::SendPacket(CPacketsID::PLAYER_WANTED_LEVEL, &packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE);
    m_nLastWantedLevelSent = 0;
}

int CStatsSync::GetSyncIdByInternal(eStats stat)
{
    for (int i = 0; i < CStatsSync::SYNCED_STATS_COUNT; i++)
    {
        if (m_aeSyncedStats[i] == stat)
            return i;
    }

    return -1;
}
