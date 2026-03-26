#include "stdafx.h"
#include "CSniperAimMarkerSync.h"

namespace
{
	struct SniperAimRenderState
	{
		CVector smoothedPos{};
		CVector targetPos{};
		uint32_t lastTick = 0;
		bool visible = false;
		float shotSizeMultiplier = 1.0f;
	};

	std::array<SniperAimRenderState, MAX_SERVER_PLAYERS> g_states{};
}

void CSniperAimMarkerSync::HandleRemoteState(const CPackets::PlayerSniperAimMarkerState& packet)
{
	if (packet.playerid < 0 || packet.playerid >= MAX_SERVER_PLAYERS)
	{
		return;
	}

	auto& state = g_states[packet.playerid];
	state.targetPos = packet.source + (packet.direction * packet.range);
	if (state.lastTick == 0 || !state.visible)
	{
		state.smoothedPos = state.targetPos;
	}
	state.lastTick = packet.tick;
	state.visible = packet.visible;
}

void CSniperAimMarkerSync::ApplyShotEvent(int playerId, float shotSizeMultiplier, const CVector& startPos, const CVector& endPos)
{
	if (playerId < 0 || playerId >= MAX_SERVER_PLAYERS)
	{
		return;
	}

	auto& state = g_states[playerId];
	state.shotSizeMultiplier = std::max(1.0f, shotSizeMultiplier);
	state.targetPos = endPos;
	state.smoothedPos = endPos;
	state.lastTick = GetTickCount();
	state.visible = true;
}

void CSniperAimMarkerSync::Process()
{
	if (!CNetwork::m_bConnected)
	{
		return;
	}

	const uint32_t nowTick = GetTickCount();
	for (int i = 0; i < MAX_SERVER_PLAYERS; i++)
	{
		if (i == CNetworkPlayerManager::m_nMyId)
		{
			continue;
		}

		auto* networkPlayer = CNetworkPlayerManager::GetPlayer(i);
		auto& state = g_states[i];
		if (!networkPlayer || !networkPlayer->m_pPed || !state.visible)
		{
			continue;
		}

		const float smoothingAlpha = 0.18f;
		state.smoothedPos = state.smoothedPos + ((state.targetPos - state.smoothedPos) * smoothingAlpha);

		RwV3d screenPos{};
		float w = 0.0f;
		float h = 0.0f;
		if (!CSprite::CalcScreenCoors({ state.smoothedPos.x, state.smoothedPos.y, state.smoothedPos.z }, &screenPos, &w, &h, true, true))
		{
			continue;
		}

		const float radius = std::clamp(2.0f * state.shotSizeMultiplier, 2.0f, 8.0f);
		CRect rect(
			screenPos.x - radius,
			screenPos.y - radius,
			screenPos.x + radius,
			screenPos.y + radius);
		CSprite2d::DrawRect(rect, CRGBA(255, 32, 32, 220));

		if (state.shotSizeMultiplier > 1.0f)
		{
			state.shotSizeMultiplier = std::max(1.0f, state.shotSizeMultiplier - 0.03f);
		}

		if (nowTick > state.lastTick + 500)
		{
			state.visible = false;
		}
	}
}
