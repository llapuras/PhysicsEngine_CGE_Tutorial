#include "PositionConstraint.h"
#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "Debug.h"

using namespace NCL;
using namespace NCL::Maths;
using namespace CSC8503;

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d)
{
	objectA		= a;
	objectB		= b;
	distance	= d;
}

PositionConstraint::~PositionConstraint()
{

}

//a simple constraint that stops objects from being more than <distance> away
//from each other...this would be all we need to simulate a rope, or a ragdoll
void PositionConstraint::UpdateConstraint(float dt) {
	Vector3 relativePos =
		objectA->GetConstTransform().GetWorldPosition() - objectB->GetConstTransform().GetWorldPosition();

	float currentDistance = relativePos.Length();
	float offset = distance - currentDistance;

	if (abs(offset) > 0) {

		Vector3 offsetDir = relativePos.Normalised();

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();

		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

		if (constraintMass > 0.0f) {
			// how much of their relative force is affecting the constraint
			float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;
			
			float lambda = -(velocityDot + bias) / constraintMass;
			
			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;
			
			physA -> ApplyLinearImpulse(aImpulse); // multiplied by mass here
			physB -> ApplyLinearImpulse(bImpulse); // multiplied by mass here
		}
	}
}

//-----------------------------------------------------------------------
//add new type constraint → fixed constraint
//-----------------------------------------------------------------------
FixedConstraint::FixedConstraint(GameObject* a, GameObject* b) {
	objectA = a;
	objectB = b;
}

FixedConstraint::~FixedConstraint() {

}

void FixedConstraint::UpdateConstraint(float dt) {
	//计算相对位置，一旦物体触碰，则物体B会固定到物体A上
	Vector3 relativePos =
		objectA->GetConstTransform().GetWorldPosition() - objectB->GetConstTransform().GetWorldPosition();

	//一旦监测到大鹅和苹果碰撞，就把苹果加到大鹅嘴巴的位置，不像positionconstraint，不会有多余的力被施加
	//只是两个物体的相对位置不再发生变化
	PhysicsObject* physA = objectA->GetPhysicsObject();
	PhysicsObject* physB = objectB->GetPhysicsObject();

	//同步物体A的速度和加速度到物体B（到这一步是ok的）
	physB->SetLinearVelocity(physA->GetLinearVelocity());
	physB->SetAngularVelocity(physA->GetAngularVelocity());
	
}