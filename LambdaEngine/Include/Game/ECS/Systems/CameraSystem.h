#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "Application/API/Events/KeyEvents.h"

namespace LambdaEngine
{
	class LAMBDA_API CameraSystem : public System
	{
	public:
		DECL_REMOVE_COPY(CameraSystem);
		DECL_REMOVE_MOVE(CameraSystem);
		~CameraSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override;

		void MainThreadTick(Timestamp deltaTime);

		FORCEINLINE void SetMainFOV(float32 fov) { m_MainFOV = fov; }
		FORCEINLINE float32 GetMainFOV() const { return m_MainFOV; }

	public:
		static CameraSystem& GetInstance() { return s_Instance; }

	private:
		CameraSystem() = default;

		bool OnKeyPressed(const KeyPressedEvent& event);
		// MoveFreeCamera translates and rotates a free camera
		void MoveFreeCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FreeCameraComponent& freeCamComp);
		// MoveFPSCamera translates and rotates an FPS camera
		void MoveFPSCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FPSControllerComponent& FPSComp);
		void RotateCamera(float32 dt, const glm::vec3& forward, glm::quat& rotation);
		void RenderFrustum(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp);

	private:
		IDVector	m_AttachedCameraEntities;
		IDVector	m_CameraEntities;
		bool		m_CIsPressed	= false;
		bool		m_MouseEnabled	= false;

		bool		m_VisbilityChanged = false;
		glm::ivec2	m_NewMousePos;

		THashTable<Entity, uint32> m_LineGroupEntityIDs;

		float32		m_MainFOV = 0.0f;

	private:
		static CameraSystem	s_Instance;
	};
}