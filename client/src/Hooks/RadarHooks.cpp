#include "stdafx.h"
#include "RadarHooks.h"
#include "CNetworkStaticBlip.h"
#include <CEntryExit.h>

CVector RadarHooks::NormalizeMapPinPosition(const CVector& position)
{
	CVector normalized = position;
	constexpr float mapWorldRange = 3000.0f;
	normalized.x = std::clamp(normalized.x, -mapWorldRange, mapWorldRange);
	normalized.y = std::clamp(normalized.y, -mapWorldRange, mapWorldRange);
	normalized.z = std::clamp(normalized.z, -500.0f, 2000.0f);

	const float aspect = static_cast<float>(RsGlobal.maximumWidth) / static_cast<float>(std::max(1, RsGlobal.maximumHeight));
	const float aspectSafe = std::clamp(aspect, 1.0f, 3.0f);
	const float normalizedX = normalized.x / mapWorldRange;
	const float normalizedY = normalized.y / mapWorldRange;
	normalized.x = std::clamp((normalizedX / aspectSafe) * mapWorldRange * aspectSafe, -mapWorldRange, mapWorldRange);
	normalized.y = std::clamp(normalizedY * mapWorldRange, -mapWorldRange, mapWorldRange);

	return normalized;
}

void CheckAndMarkRadarSync(int blipHandle)
{
	if (!CLocalPlayer::m_bIsHost)
		return;

	if (const auto index = CRadar::GetActualBlipArrayIndex(blipHandle); index != -1)
	{
		if (CNetworkStaticBlip::IsAllowedSyncingRadarSprite(static_cast<eRadarSprite>(CRadar::ms_RadarTrace[index].m_nRadarSprite)))
		{
			CNetworkStaticBlip::ms_bNeedToSendAfterThisFrame = true;
		}
	}
}

void CRadar__SetBlipSprite_Hook(int blipHandle, char spriteId)
{
	CRadar::SetBlipSprite(blipHandle, spriteId);
	CheckAndMarkRadarSync(blipHandle);
}

int CRadar__SetCoordBlip_Hook(eBlipType type, CVector posn, int a5, eBlipDisplay display)
{
	int result = CRadar::SetCoordBlip(type, RadarHooks::NormalizeMapPinPosition(posn), a5, display, nullptr);
	CheckAndMarkRadarSync(result);
	return result;
}

void CRadar__ClearBlip_Hook(int blipHandle)
{
	CheckAndMarkRadarSync(blipHandle);
	CRadar::ClearBlip(blipHandle);
}

void CRadar__ChangeBlipColour_Hook(int blipHandle, int color)
{
	CRadar::ChangeBlipColour(blipHandle, color);
	CheckAndMarkRadarSync(blipHandle);
}

void CRadar__SetCoordBlipAppearance_Hook(int blipHandle, uint8_t appearance)
{
	CRadar::SetCoordBlipAppearance(blipHandle, appearance);
	CheckAndMarkRadarSync(blipHandle);
}

void CRadar__SetBlipFriendly_Hook(int blipHandle, bool friendly)
{
	CRadar::SetBlipFriendly(blipHandle, friendly);
	CheckAndMarkRadarSync(blipHandle);
}

void CRadar__SetBlipEntryExit_Hook(int blipHandle, CEntryExit* enex)
{
	CRadar::SetBlipEntryExit(blipHandle, enex);
	CheckAndMarkRadarSync(blipHandle);
}

void RadarHooks::InjectHooks()
{
	patch::RedirectCall(0x444403, CRadar__SetBlipSprite_Hook);
	patch::RedirectCall(0x47F7FE, CRadar__SetBlipSprite_Hook);
	patch::RedirectCall(0x48BCA8, CRadar__SetBlipSprite_Hook);
	patch::RedirectCall(0x48DBE1, CRadar__SetBlipSprite_Hook);

	patch::RedirectCall(0x47C744, CRadar__SetCoordBlip_Hook);
	patch::RedirectCall(0x47CFE0, CRadar__SetCoordBlip_Hook);
	patch::RedirectCall(0x47F74C, CRadar__SetCoordBlip_Hook);
	patch::RedirectCall(0x58394E, CRadar__SetCoordBlip_Hook);

	patch::RedirectCall(0x446EC0, CRadar__ClearBlip_Hook);
	patch::RedirectCall(0x47C670, CRadar__ClearBlip_Hook);

	patch::RedirectCall(0x47C698, CRadar__ChangeBlipColour_Hook);

	patch::RedirectCall(0x476114, CRadar__SetCoordBlipAppearance_Hook);

	patch::RedirectCall(0x47285D, CRadar__SetBlipFriendly_Hook);

	patch::RedirectCall(0x475882, CRadar__SetBlipEntryExit_Hook);
}
