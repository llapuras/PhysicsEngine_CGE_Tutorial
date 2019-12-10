#pragma once
#include "../../Common/Vector3.h"

using namespace NCL::Maths;

namespace NCL {
	enum class BoundingType {
		AABB,
		OOBB,
		Sphere,
		Capsule,
		Mesh
	};

	class BoundingVolume
	{
	public:
		BoundingVolume();
		~BoundingVolume();


		BoundingType type;
	};
}

