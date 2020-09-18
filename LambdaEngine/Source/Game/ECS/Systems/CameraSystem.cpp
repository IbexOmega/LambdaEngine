#include "Game/ECS/Systems/CameraSystem.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include <glm/gtx/euler_angles.hpp>
#include "Input/API/Input.h"
#include "Log/Log.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

namespace LambdaEngine
{
	CameraSystem CameraSystem::s_Instance;

	//The up vector is inverted because of vulkans inverted y-axis
	const glm::vec3 UP_VECTOR = glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 FORWARD_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);

	bool CameraSystem::Init()
	{
		TransformComponents transformComponents;
		transformComponents.Position.Permissions	= R;
		transformComponents.Scale.Permissions		= R;
		transformComponents.Rotation.Permissions	= R;

		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{RW, CameraComponent::s_TID}, {RW, ViewProjectionMatricesComponent::s_TID}, {RW, FreeCameraComponent::s_TID}}, {&transformComponents}, &m_CameraEntities}
			};
			systemReg.Phase = g_LastPhase-1;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void CameraSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<CameraComponent>* pCameraComponents = pECSCore->GetComponentArray<CameraComponent>();

		for (Entity entity : m_CameraEntities.GetIDs())
		{
			auto& camComp = pCameraComponents->GetData(entity);
			if (camComp.IsActive)
			{
				auto& viewProjComp = pECSCore->GetComponent<ViewProjectionMatricesComponent>(entity);
				auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
				auto& rotComp = pECSCore->GetComponent<RotationComponent>(entity);

				TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
				const uint16 width = window->GetWidth();
				const uint16 height = window->GetHeight();
				camComp.Jitter = glm::vec2((Random::Float32() - 0.5f) / (float)width, (Random::Float32() - 0.5f) / (float)height);
				HandleInput(deltaTime, entity, camComp, viewProjComp, posComp, rotComp);
			}
		}
	}

	void CameraSystem::MainThreadTick(Timestamp)
	{
		if (m_VisbilityChanged)
		{
			CommonApplication::Get()->SetMouseVisibility(!m_MouseEnabled);
			m_VisbilityChanged = false;
		}

		if (m_MouseEnabled)
		{
			CommonApplication::Get()->SetMousePosition(m_NewMousePos.x, m_NewMousePos.y);
		}
	}

	void CameraSystem::HandleInput(Timestamp delta, Entity entity, CameraComponent& camComp, ViewProjectionMatricesComponent& viewProjComp, PositionComponent& posComp, RotationComponent& rotComp)
	{
		const auto& freeCamComp = ECSCore::GetInstance()->GetComponent<FreeCameraComponent>(entity);

		constexpr float CAMERA_MOUSE_SPEED = 10.0f;

		float32 dt = float32(delta.AsSeconds());

		glm::vec3 translation = {
				float(Input::IsKeyDown(EKey::KEY_D) - Input::IsKeyDown(EKey::KEY_A)),	// X: Right
				float(Input::IsKeyDown(EKey::KEY_Q) - Input::IsKeyDown(EKey::KEY_E)),	// Y: Up
				float(Input::IsKeyDown(EKey::KEY_W) - Input::IsKeyDown(EKey::KEY_S))	// Z: Forward
		};

		const glm::vec3 forward = GetForward(rotComp.Quaternion);
		const glm::vec3 right	= GetRight(rotComp.Quaternion);

		if (glm::length2(translation) > glm::epsilon<float>())
		{
			const float shiftSpeedFactor = Input::IsKeyDown(EKey::KEY_LEFT_SHIFT) ? 2.0f : 1.0f;
			translation = glm::normalize(translation) * freeCamComp.SpeedFactor * shiftSpeedFactor * dt;

			posComp.Position += translation.x * right + translation.y * GetUp(rotComp.Quaternion) + translation.z * forward;
		}

		// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
		float addedPitch	= dt * float(Input::IsKeyDown(EKey::KEY_DOWN) - Input::IsKeyDown(EKey::KEY_UP));
		float addedYaw		= dt * float(Input::IsKeyDown(EKey::KEY_LEFT) - Input::IsKeyDown(EKey::KEY_RIGHT));

		if (Input::IsKeyDown(EKey::KEY_C))
		{
			if (!m_CIsPressed)
			{
				m_MouseEnabled		= !m_MouseEnabled;
				m_VisbilityChanged	= true;
				m_CIsPressed		= true;
			}
		}
		else if (Input::IsKeyUp(EKey::KEY_C))
		{
			m_CIsPressed = false;
		}

		if (m_MouseEnabled)
		{
			const MouseState& mouseState = Input::GetMouseState();

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const uint16 width = window->GetWidth();
			const uint16 height = window->GetHeight();

			const glm::vec2 mouseDelta(mouseState.Position.x - (int)(width * 0.5), mouseState.Position.y - (int)(height * 0.5));
			m_NewMousePos = glm::ivec2((int)(width * 0.5), (int)(height * 0.5));

			if (glm::length(mouseDelta) > glm::epsilon<float>())
			{
				addedYaw	-= freeCamComp.MouseSpeedFactor * (float)mouseDelta.x * dt;
				addedPitch	+= freeCamComp.MouseSpeedFactor * (float)mouseDelta.y * dt;
			}
		}

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		const float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		const float currentYaw = GetYaw(forward) + addedYaw;

		rotComp.Quaternion =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch

		viewProjComp.View = glm::lookAt(posComp.Position, posComp.Position + GetForward(rotComp.Quaternion), g_DefaultUp);
		camComp.ViewInv = glm::inverse(viewProjComp.View);
	}
}
