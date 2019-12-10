#pragma once
#include "CollisionVolume.h"

namespace NCL {
	class CapsuleVolume : CollisionVolume
	{
	public:
		CapsuleVolume(float capsuleRadius = 1.0f) {
			type = VolumeType::Capsule;
			radius = capsuleRadius;

		}
		~CapsuleVolume() {}

		float GetRadius() const {
			return radius;
		}

		float GetHeight() const {
			return height;
		}

	protected:
		float	radius;
		float height;

	};
}

