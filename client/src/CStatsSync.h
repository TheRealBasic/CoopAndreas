#pragma once
class CStatsSync
{
public:
	static constexpr inline int SYNCED_STATS_COUNT = 14;

	static std::array<eStats, SYNCED_STATS_COUNT> m_aeSyncedStats;
	static inline int m_nLastWantedLevelSent = -1;
	static void ApplyNetworkPlayerContext(CNetworkPlayer* player);
	static void ApplyLocalContext();
	static void NotifyChanged();
	static void NotifyWantedLevelChanged();
	static void TriggerWantedLevelReset();
	static int GetSyncIdByInternal(eStats stat);
};
