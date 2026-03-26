#pragma once
#include "CVector.h"
class RadarHooks
{
public:
	static CVector NormalizeMapPinPosition(const CVector& position);
	static void InjectHooks();
};
