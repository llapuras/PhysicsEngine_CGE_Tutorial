#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../../Common/Assets.h"
#include "../CSC8503Common/NavigationGrid.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			void AChasedByB(GameObject* a, GameObject* b);
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:
			void InitialiseAssets();

			//添加照相机跟随选项
			void InitCamera(GameObject* target = nullptr);
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void BridgeConstraintTest();
			void SimpleGJKTest();

			bool SelectObject();
			void MoveSelectedObject();
			bool DebugObjectMovement(); //状态机改成bool
			void LockedObjectMovement();
			void LockedCameraMovement();
			void InitMaze();


			//新功能
			void DrawMaze(const std::string& filename);  //生成地图
			void AChasedByB(GameObject* a, GameObject* b, float minDistant = 10);  //生成追赶寻路,距离x单位长度开始追逐

			//状态机
			void GooseGameMenuStateMachine();
			void GooseStateMachine();
			void KeeperStateMachine();

			int gooseData = 0;
			int keeperData = 0;
			int menuData = 0;

			//StateMachine* gooseStateMachine;


			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, float elasticity = 1.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddBlockToWorld(const Vector3& position);
			GameObject* AddWallToWorld(const Vector3& position);

			GameObject* TutorialGame::AddArea(const string name, const Vector3& position, const Vector3& areasize = Vector3(1, 0.1, 1));

			//IT'S HAPPENING
			GameObject* AddGooseToWorld(const Vector3& position);
			GameObject* AddParkKeeperToWorld(const Vector3& position);
			GameObject* AddCharacterToWorld(const Vector3& position);
			GameObject* AddAppleToWorld(const Vector3& position);


			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLTexture* blockTex = nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	gooseMesh	= nullptr;
			OGLMesh*	keeperMesh	= nullptr;
			OGLMesh*	appleMesh	= nullptr;
			OGLMesh*	charA		= nullptr;
			OGLMesh*	charB		= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 16, -20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}
		};
	}
}

