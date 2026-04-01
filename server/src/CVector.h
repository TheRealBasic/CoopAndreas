#pragma once

#ifndef _CVECTOR_H_
	#define _CVECTOR_H_

#include <cmath>

struct CVector
{
	public:
		float x;
		float y;
		float z;

		CVector()
		{
			x = y = z = 0.0f;
		}

		CVector(float x, float y, float z)
			{
				this->x = x;
				this->y = y;
				this->z = z;
			}

			CVector operator-(const CVector& rhs) const
			{
				return CVector(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
			}

			void Normalise()
			{
				const float lengthSquared = (x * x) + (y * y) + (z * z);
				if (lengthSquared <= 0.0f)
				{
					return;
				}

				const float invLength = 1.0f / std::sqrt(lengthSquared);
				x *= invLength;
				y *= invLength;
				z *= invLength;
			}
};

#endif
