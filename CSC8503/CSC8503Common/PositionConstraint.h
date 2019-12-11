#pragma once
#include "Constraint.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class PositionConstraint : public Constraint	{
		public:
			PositionConstraint(GameObject* a, GameObject* b, float d);
			~PositionConstraint();

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;

			float distance;
		};

		//
		class FixedConstraint : public Constraint {
		public:
			FixedConstraint(GameObject* a, GameObject* b);
			~FixedConstraint();

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;

			////each object's connect point's position
			//Vector3 positionA;
			//Vector3 positionB;
		};


		class AxisesFreeze :public Constraint {
		public:
		/*	PositionFreeze(Vector3 freezeinfo);*/


		};
	}
}