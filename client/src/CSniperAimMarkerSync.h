#pragma once

class CSniperAimMarkerSync
{
public:
	static void HandleRemoteState(const CPackets::PlayerSniperAimMarkerState& packet);
	static void ApplyShotEvent(int playerId, float shotSizeMultiplier, const CVector& startPos, const CVector& endPos);
	static void Process();
};
