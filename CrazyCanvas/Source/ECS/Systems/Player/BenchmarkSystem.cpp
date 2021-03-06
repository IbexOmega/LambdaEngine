#include "ECS/Systems/Player/BenchmarkSystem.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Input/API/Input.h"

#include "Physics/PhysicsEvents.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Multiplayer/Packet/PacketWeaponFired.h"

void BenchmarkSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		// The write permissions are used when creating projectile entities
		PlayerGroup playerGroup;
		playerGroup.Rotation.Permissions = RW;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_WeaponEntities,
				.ComponentAccesses =
				{
					{ RW, WeaponComponent::Type() },
					{ R, PositionComponent::Type() },
					{ R, RotationComponent::Type() },
				}
			},
			{
				.pSubscriber = &m_LocalPlayerEntities,
				.ComponentAccesses =
				{
					{ RW, CharacterColliderComponent::Type() },
					{ R, NetworkPositionComponent::Type() },
					{ RW, VelocityComponent::Type() }
				},
				.ComponentGroups = { &playerGroup }
			}
		};
		systemReg.Phase = 0u;

		RegisterSystem(TYPE_NAME(BenchmarkSystem), systemReg);
	}
}

void BenchmarkSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<WeaponComponent>*	pWeaponComponents	= pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<RotationComponent>*	pRotationComponents	= pECS->GetComponentArray<RotationComponent>();

	/* Component arrays for moving character colliders */
	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents	= pECS->GetComponentArray<CharacterColliderComponent>();
	const ComponentArray<NetworkPositionComponent>* pNetworkPositionComponents	= pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents						= pECS->GetComponentArray<VelocityComponent>();

	WeaponSystem& weaponSystem = WeaponSystem::GetInstance();
	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		const Entity playerEntity = weaponComponent.WeaponOwner;
		if (!m_LocalPlayerEntities.HasElement(playerEntity))
		{
			continue;
		}

		// Rotate player
		constexpr const float32 rotationSpeed = 3.14f / 4.0f;
		RotationComponent& rotationComp = pRotationComponents->GetData(playerEntity);
		CharacterColliderComponent& characterColliderComp = pCharacterColliderComponents->GetData(playerEntity);
		const NetworkPositionComponent& networkPositionComp = pNetworkPositionComponents->GetConstData(playerEntity);
		VelocityComponent& velocityComp = pVelocityComponents->GetData(playerEntity);
		rotationComp.Quaternion = glm::rotate(rotationComp.Quaternion, rotationSpeed * dt, g_DefaultUp);

		// Move character controller to apply gravity and to stress the physics system
		CharacterControllerHelper::TickCharacterController(dt, characterColliderComp, networkPositionComp, velocityComp);

		/* Fire weapon if appropriate */
		if (weaponComponent.CurrentCooldown > 0.0f)
		{
			weaponComponent.CurrentCooldown -= dt;
			continue;
		}

		//Calculate Weapon Fire Properties (Position, Velocity and Team)
		glm::vec3 firePosition;
		glm::vec3 fireVelocity;
		uint8 playerTeam;
		weaponSystem.CalculateWeaponFireProperties(weaponEntity, firePosition, fireVelocity, playerTeam);

		weaponSystem.Fire(weaponEntity, weaponComponent, EAmmoType::AMMO_TYPE_PAINT, firePosition, fireVelocity, playerTeam, 0);
		weaponComponent.CurrentCooldown = 1.0f / weaponComponent.FireRate;
	}
}
