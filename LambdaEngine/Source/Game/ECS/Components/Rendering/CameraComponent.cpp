#include "Audio/AudioAPI.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Audio/ListenerComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	Entity CreateFreeCameraEntity(const CameraDesc& cameraDesc)
	{
		Entity entity = CreateCameraEntity(cameraDesc);

		FreeCameraComponent freeCamComp;
		ECSCore::GetInstance()->AddComponent<FreeCameraComponent>(entity, freeCamComp);

		return entity;
	}

	Entity CreateFPSCameraEntity(const CameraDesc& cameraDesc)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = CreateCameraEntity(cameraDesc);

		FPSControllerComponent FPSCamComp;
		FPSCamComp.SpeedFactor = 2.4f;
		pECS->AddComponent<FPSControllerComponent>(entity, FPSCamComp);

		const CharacterColliderCreateInfo colliderInfo = {
			.Entity		= entity,
			.Position	= pECS->GetComponent<PositionComponent>(entity),
			.Rotation	= pECS->GetComponent<RotationComponent>(entity),
			.CollisionGroup = FCollisionGroup::COLLISION_GROUP_DYNAMIC,
			.CollisionMask	= UINT32_MAX, // The player collides with everything
			.EntityID		= entity
		};

		constexpr const float capsuleHeight = 1.8f;
		constexpr const float capsuleRadius = 0.2f;
		CharacterColliderComponent characterColliderComponent = PhysicsSystem::GetInstance()->CreateCharacterCapsule(colliderInfo, std::max(0.0f, capsuleHeight - 2.0f * capsuleRadius), capsuleRadius);
		pECS->AddComponent<CharacterColliderComponent>(entity, characterColliderComponent);

		// Listener
		pECS->AddComponent<ListenerComponent>(entity, { AudioAPI::GetDevice()->GetAudioListener(false) });

		return entity;
	}

	Entity CreateCameraTrackEntity(const LambdaEngine::CameraDesc& cameraDesc, const TArray<glm::vec3>& track)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = CreateCameraEntity(cameraDesc);

		TrackComponent camTrackComp;
		camTrackComp.Track = track;
		pECS->AddComponent<TrackComponent>(entity, camTrackComp);

		return entity;
	}

	Entity CreateCameraEntity(const CameraDesc& cameraDesc)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const Entity entity = pECS->CreateEntity();

		const ViewProjectionMatricesComponent viewProjComp = {
			.Projection = glm::perspective(glm::radians(cameraDesc.FOVDegrees), cameraDesc.Width / cameraDesc.Height, cameraDesc.NearPlane, cameraDesc.FarPlane),
			.View = glm::lookAt(cameraDesc.Position, cameraDesc.Position + cameraDesc.Direction, g_DefaultUp)
		};
		pECS->AddComponent<ViewProjectionMatricesComponent>(entity, viewProjComp);

		const CameraComponent camComp = {
			.NearPlane	= cameraDesc.NearPlane,
			.FarPlane	= cameraDesc.FarPlane,
			.FOV		= cameraDesc.FOVDegrees
		};
		pECS->AddComponent<CameraComponent>(entity, camComp);

		pECS->AddComponent<PositionComponent>(entity, PositionComponent{ .Position = cameraDesc.Position });
		pECS->AddComponent<RotationComponent>(entity, RotationComponent{ .Quaternion = glm::quatLookAt(cameraDesc.Direction, g_DefaultUp) });
		pECS->AddComponent<ScaleComponent>(entity, ScaleComponent{ .Scale = {1.f, 1.f, 1.f} });
		pECS->AddComponent<VelocityComponent>(entity, VelocityComponent());

		return entity;
	}
}
