#include "World/Player/Client/PlayerForeignSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "ECS/ECSCore.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Multiplayer/Packet/PacketPlayerActionResponse.h"

#include "World/Player/Client/PlayerSoundHelper.h"

#include "Match/Match.h"

using namespace LambdaEngine;

PlayerForeignSystem::PlayerForeignSystem()
{
}

PlayerForeignSystem::~PlayerForeignSystem()
{
}

void PlayerForeignSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_Entities,
			.ComponentAccesses =
			{
				{NDA, PlayerForeignComponent::Type()},
				{RW, CharacterColliderComponent::Type()},
				{RW, NetworkPositionComponent::Type()},
				{R , PositionComponent::Type()},
				{RW, VelocityComponent::Type()},
				{RW, RotationComponent::Type()},
				{R, PacketComponent<PacketPlayerActionResponse>::Type()},
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(TYPE_NAME(PlayerForeignSystem), systemReg);
}

void PlayerForeignSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<NetworkPositionComponent>* pNetPosComponents						= pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<CharacterColliderComponent>* pCharacterColliders							= pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents									= pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents							= pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents									= pECS->GetComponentArray<RotationComponent>();
	ComponentArray<AudibleComponent>* pAudibleComponent										= pECS->GetComponentArray<AudibleComponent>();
	const ComponentArray<PacketComponent<PacketPlayerActionResponse>>* pPacketComponents	= pECS->GetComponentArray<PacketComponent<PacketPlayerActionResponse>>();

	for (Entity entity : m_Entities)
	{
		const NetworkPositionComponent& constNetPosComponent	= pNetPosComponents->GetConstData(entity);
		const PositionComponent& positionComponent				= pPositionComponents->GetConstData(entity);
		VelocityComponent& velocityComponent					= pVelocityComponents->GetData(entity);
		AudibleComponent& audibleComponent						= pAudibleComponent->GetData(entity);
		CharacterColliderComponent& characterColliderComponent	= pCharacterColliders->GetData(entity);

		physx::PxControllerState playerControllerState;
		characterColliderComponent.pController->getState(playerControllerState);
		bool inAir = (playerControllerState.touchedShape == nullptr);

		const PacketComponent<PacketPlayerActionResponse>& packetComponent	= pPacketComponents->GetConstData(entity);
		const TArray<PacketPlayerActionResponse>& gameStates = packetComponent.GetPacketsReceived();

		if (!gameStates.IsEmpty())
		{
			const PacketPlayerActionResponse& latestGameState = gameStates.GetBack();

			const RotationComponent& constRotationComponent = pRotationComponents->GetConstData(entity);

			if (constRotationComponent.Quaternion != latestGameState.Rotation)
			{
				RotationComponent& rotationComponent = const_cast<RotationComponent&>(constRotationComponent);
				rotationComponent.Quaternion	= latestGameState.Rotation;
				rotationComponent.Dirty			= true;
			}

			if (glm::any(glm::notEqual(constNetPosComponent.Position, latestGameState.Position)))
			{
				NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
				netPosComponent.PositionLast	= positionComponent.Position;
				netPosComponent.Position		= latestGameState.Position;
				netPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();
				netPosComponent.Dirty			= true;
			}

			velocityComponent.Velocity = latestGameState.Velocity;

			CharacterControllerHelper::SetForeignCharacterController(characterColliderComponent, constNetPosComponent);
		}
		else if(Match::HasBegun()) //Data does not exist for the current frame :(
		{
			float32 dt = (float32)deltaTime.AsSeconds();
			CharacterControllerHelper::ApplyGravity(dt, velocityComponent.Velocity);

			NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
			netPosComponent.PositionLast = positionComponent.Position;
			netPosComponent.Position += velocityComponent.Velocity * dt;
			netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();
			netPosComponent.Dirty = true;

			CharacterControllerHelper::TickForeignCharacterController(dt, characterColliderComponent, constNetPosComponent, velocityComponent);
		}

		const PacketPlayerActionResponse& lastReceivedGameState = packetComponent.GetLastReceivedPacket();
		PlayerSoundHelper::HandleMovementSound(
			velocityComponent,
			audibleComponent,
			lastReceivedGameState.Walking,
			inAir,
			characterColliderComponent.WasInAir);

		characterColliderComponent.WasInAir = inAir;
	}
}