#include "TutorialGame.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/PositionConstraint.h"
#include <fstream>

using namespace NCL;
using namespace CSC8503;

GameObject* gugugu;
GameObject* man;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}


vector < Vector3 > testNodes02;
float ChaseSpeed = 0.03f;
void TutorialGame::AChasedByB(GameObject* a, GameObject* b, float minDistant) {
	
	NavigationGrid grid("TestGrid1.txt");

	NavigationPath outPath;

	Vector3 startPos = a -> GetTransform().GetWorldPosition();
	Vector3 endPos = b -> GetTransform().GetWorldPosition();
	Vector3 relativePos = startPos - endPos;
	
	if (relativePos.Length() < minDistant) {
		//追逐者移动方向
		Vector3 chaseDir = Vector3(ChaseSpeed, ChaseSpeed, ChaseSpeed) * (startPos - endPos).Normalised();
		b->GetTransform().SetWorldPosition(endPos + chaseDir);
	}
	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes02.push_back(pos);
	}



	//debug用
	Debug::EraseLines();//擦掉之前的线
	for (int i = 1; i < testNodes02.size(); ++i) {
		Vector3 posA = testNodes02[i - 1];
		Vector3 posB = testNodes02[i];

		Debug::DrawLine(posA, posB, Vector4(1, 0, 0, 1));
	}

}


/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("goose.msh"	 , &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh"	 , &appleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	blockTex    = (OGLTexture*)TextureLoader::LoadAPITexture("blockTex.png");

	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete gooseMesh;
	delete basicTex;
	delete blockTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		LockedCameraMovement();
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(10, 40));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(10, 40));
	}

	SelectObject();
	MoveSelectedObject();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	Debug::FlushRenderables();
	renderer->Render();

	//Debug::DrawLine(gugugu->GetTransform().GetWorldPosition(), man->GetTransform().GetWorldPosition(), Vector4(1, 0, 0, 1));
	testNodes02.clear();
	AChasedByB(gugugu, man, 20);

	//状态机检查
	GooseGameMenuStateMachine();
	GooseStateMachine();
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(gugugu); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis);
		gooseData = 1; //状态机
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddForce(rightAxis);
		gooseData = 1; //状态机
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
		gooseData = 1; //状态机
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
		gooseData = 1; //状态机
	}
}

void  TutorialGame::LockedCameraMovement() {
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetWorldPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);
	}
}


bool TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!

		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		//	selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		//}

		gooseData = 0; //状态机

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::T)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::R)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}


		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -forceMagnitude));
			gooseData = 1; //状态机
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-forceMagnitude, 0, 0));
			gooseData = 1; //状态机
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(forceMagnitude, 0, 0));
			gooseData = 1; //状态机
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, forceMagnitude));
			gooseData = 1; //状态机
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -forceMagnitude, 0));
		}
	
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, forceMagnitude, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -forceMagnitude, 0));
		}
	}
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool TutorialGame::SelectObject() {

	renderer->DrawString("Points: 12", Vector2(10, 650), Vector4(0,0.3,1,1));

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}

	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(10, 0));
		//if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
		//	if (selectionObject) {	//set colour to deselected;
		selectionObject = gugugu;
		selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));

		//	}
		//	
		//	Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		//	RayCollision closestCollision;
		//	if (world->Raycast(ray, closestCollision, true)) {
		//		selectionObject = (GameObject*)closestCollision.node;
		//		selectionObject->GetRenderObject()->SetColour(Vector4(0, 0, 0.8, 1));
		//		return true;
		//	}
		//	else {
		//		return false;
		//	}
		//}




		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	renderer -> DrawString("Click Force :" + std::to_string(forceMagnitude), Vector2(10, 20)); // Draw debug text at 10 ,20
	forceMagnitude += Window::GetMouse() -> GetWheelMovement() * 100.0f;
	
	if (!selectionObject) {
		return;// we haven ’t selected anything !
	}
	// Push the selected object !
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}

//添加摄像机target跟随
void TutorialGame::InitCamera(GameObject* target){
	world->GetMainCamera()->SetNearPlane(0.5f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = target;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	//BridgeConstraintTest();
	
	//SimpleGJKTest();
	//InitSphereGridWorld(3, 3, 3.5f, 3.5f, 2.0f);
	//InitMixedGridWorld(10, 10, 3.5f, 3.5f);
	gugugu = AddGooseToWorld(Vector3(15, 15, 22));
	gugugu->GetPhysicsObject()->SetInverseMass(10);
	
	//AddAppleToWorld(Vector3(42, 15, 32));
	//AddAppleToWorld(Vector3(70, 15, 72));

	//AddSphereToWorld(Vector3(42, 15, 22), 4.0, 0.1, 0.1);

	//AddSphereToWorld(Vector3(32, 15, 22), 2.0, 0.1, 3);


	man = AddParkKeeperToWorld(Vector3(70, 15, 72));
	//AddCharacterToWorld(Vector3(45, 2, 0));
	AddArea("Home", Vector3(15,-2.9,15));

	//InitMaze();
	AddWallToWorld(Vector3(0, 9, 0));
	//AddBlockToWorld(Vector3(0, 7, 2));
	//AddBlockToWorld(Vector3(0, 7, 4));
	
	DrawMaze("TestGrid1.txt");

	AddFloorToWorld(Vector3(45, -5, 45));

	
}

void TutorialGame::InitMaze() {
	for(int i=-10;i<10;i++)
		AddWallToWorld(Vector3(i*2, 9, 0));

	AddBlockToWorld(Vector3(0, 9, 12));
	AddBlockToWorld(Vector3(0, 9, -12));
}


void TutorialGame::DrawMaze(const std::string& filename) {
	std::ifstream infile(Assets::DATADIR + filename);

	int nodeSize;
	int gridWidth;
	int gridHeight;

	Vector3 startPos(0, 2, 0);

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;
	AddWallToWorld(startPos);
	for (int y = 0; y < gridHeight; y++) {
		for (int x = 0; x < gridWidth; x++) {
			char type = 0;
			infile >> type;
			if(type == 'x')
				AddWallToWorld(startPos+Vector3(x*10, 0, y*10));
		}
	}
}

//From here on it's functions to add in objects to the world!

/*

A single function to add a large immoveable cube to the bottom of our world

*/

GameObject* TutorialGame::AddArea(const string name, const Vector3& position, const Vector3& areasize) {
	GameObject* floor = new GameObject(name);

	floor->SetLayer("Area");

	Vector3 floorSize = Vector3(5,0.1,5);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, blockTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->GetRenderObject()->SetColour(Vector4(1, 1, 0, 1));

	world->AddGameObject(floor);

	return floor;
}


GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");
	floor->SetLayer("Ground");
	Vector3 floorSize = Vector3(50, 2, 50);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddBlockToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");

	Vector3 floorSize = Vector3(5, 5, 5);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, blockTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");

	floor->SetLayer("Wall");
	Vector3 floorSize = Vector3(5, 5, 5);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, blockTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}




/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, float elasticity) {
	GameObject* sphere = new GameObject("sphere");
	//layer filter
	sphere->SetLayer("floor");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();
	sphere->GetPhysicsObject()->SetElasticity(elasticity); //add elasticity

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("cube");
	
	AABBVolume* volume = new AABBVolume(dimensions);

	//layer filter
	cube->SetLayer("floor");

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position)
{
	float size			= 1.0f;
	float inverseMass	= 1.0f;

	GameObject* goose = new GameObject("goose");
	//layer filter
	goose->SetLayer("floor");

	//换成了长方体检测
	//SphereVolume* volume = new SphereVolume(size);
	AABBVolume* volume = new AABBVolume(Vector3(1, 1, 1)*size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size,size,size) );
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitCubeInertia();
	//goose->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(goose);

	return goose;
}

GameObject* TutorialGame::AddParkKeeperToWorld(const Vector3& position)
{
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	GameObject* keeper = new GameObject("keeper");

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	keeper->SetBoundingVolume((CollisionVolume*)volume);

	keeper->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	keeper->GetTransform().SetWorldPosition(position);

	keeper->SetRenderObject(new RenderObject(&keeper->GetTransform(), keeperMesh, nullptr, basicShader));
	keeper->SetPhysicsObject(new PhysicsObject(&keeper->GetTransform(), keeper->GetBoundingVolume()));

	keeper->GetPhysicsObject()->SetInverseMass(inverseMass);
	keeper->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(keeper);

	return keeper;
}

GameObject* TutorialGame::AddCharacterToWorld(const Vector3& position) {
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	GameObject* character = new GameObject("character");

	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddAppleToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("apple");

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(4, 4, 4));
	apple->GetTransform().SetWorldPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

	world->AddGameObject(apple);

	return apple;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 30.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, std::rand()*0.1f, std::rand());
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing+20, 10.0f, z * rowSpacing+30);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(1, 1, 1);

	float	invCubeMass = 0.1;
	int		numLinks = 1;
	float	maxDistance = 1;
	float	cubeDistance = 5;

	Vector3 startPos = Vector3(0, 20, 50);

	//GameObject* start = AddCubeToWorld(startPos + Vector3(0, 10, 0), cubeSize, 0);
	//GameObject* start = AddCubeToWorld(Vector3(22, 10, 22), cubeSize, 0);
	GameObject* start = AddAppleToWorld(Vector3(22, 27, 22));
	//GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 10, 0), cubeSize, 0);
	GameObject* end = AddGooseToWorld(Vector3(22, 25, 22));
	GameObject* previous = start;

	/*for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}*/
	FixedConstraint* constraint = new FixedConstraint(end, previous);

	//PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
		std::cout << "awsl" << std::endl;
		world->RemoveConstraint(constraint);
	}

}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}


//statemachine
//状态机
void TutorialGame::GooseGameMenuStateMachine() {

	StateMachine* menuStateMachine = new StateMachine();

	StateFunc Menu = [](void* data) {
		int* realData = (int*)data;
		(*realData)++;
		std::cout << "Menu Page" << std::endl;
	};

	StateFunc MultiMode = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Menu MultiMode" << std::endl;
	};

	StateFunc SingleMode = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Menu SingleMode" << std::endl;
	};

	StateFunc GameOver = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Menu GameOver" << std::endl;
	};

}

StateMachine* gooseStateMachine = new StateMachine();
void TutorialGame::GooseStateMachine() {

	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();
	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	
	std::cout << gooseData << std::endl;


	//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
	//	selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -forceMagnitude));
	//	gooseData = 1; //状态机
	//}

	StateFunc Idle = [](void* data) {
		int* realData = (int*)data;
		(*realData) = 0;
		std::cout << "Goose Idle" << std::endl;
	};

	StateFunc Move = [](void* data) {
		int* realData = (int*)data;
		(*realData) = 1;
		std::cout << "Goose Move" << std::endl;
	};

	StateFunc Fetch = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Goose Fetch" << std::endl;
	};

	StateFunc Unfetch = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Goose unFetch" << std::endl;
	};

	StateFunc ResetPos = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Goose Reset Poition" << std::endl;
	};

	GenericState* stateIdle = new GenericState(Idle, (void*)&gooseData);
	GenericState* stateMove = new GenericState(Move, (void*)&gooseData);

	gooseStateMachine->AddState(stateIdle);
	gooseStateMachine->AddState(stateMove);

	//这里添加了两个状态
	GenericTransition <int&, int>* transitionA =
		new GenericTransition <int&, int>(
			GenericTransition <int&, int>::EqualsTransition,
			gooseData, 1, stateIdle, stateMove); //data = 1时，在行动

	GenericTransition <int&, int>* transitionB =
		new GenericTransition <int&, int>(
			GenericTransition <int&, int>::EqualsTransition,
			gooseData, 0, stateMove, stateIdle); //data = 0时，在行动


	gooseStateMachine->AddTransition(transitionA);
	gooseStateMachine->AddTransition(transitionB);

	gooseStateMachine->Update();
}

void TutorialGame::KeeperStateMachine() {
	
	StateMachine* keeperStateMachine = new StateMachine();

	StateFunc Idle = [](void* data) {
		int* realData = (int*)data;
		(*realData)++;
		std::cout << "Goose Idle" << std::endl;
	};

	StateFunc MoveToward = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Keeper MoveToward" << std::endl;
	};

	StateFunc CatchGoose = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "Keeper CatchGoose" << std::endl;
	};

	

}