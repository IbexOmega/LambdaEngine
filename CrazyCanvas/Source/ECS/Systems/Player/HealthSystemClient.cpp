#include "ECS/Systems/Player/HealthSystemClient.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

/*
* HealthSystemClient
*/

HealthSystemClient::~HealthSystemClient()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &HealthSystemClient::OnPlayerAliveUpdated);
}

void HealthSystemClient::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>*						pHealthComponents			= pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<PacketHealthChanged>>*	pHealthChangedComponents	= pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	for (Entity entity : m_HealthEntities)
	{
		HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
		PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);
		for (const PacketHealthChanged& packet : packets.GetPacketsReceived())
		{
			if (healthComponent.CurrentHealth != packet.CurrentHealth)
			{
				healthComponent.CurrentHealth = packet.CurrentHealth;

				// Is this the local player
				bool isLocal = false;
				for (Entity playerEntity : m_LocalPlayerEntities)
				{
					if (playerEntity == entity)
					{
						isLocal = true;
						break;
					}
				}

				// Send event to notify systems that a player got hit
				const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(entity);
				const glm::vec3 position = positionComp.Position;

				PlayerHitEvent event(entity, position, isLocal);
				EventQueue::SendEvent(event);
			}
		}
	}
}

bool HealthSystemClient::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= NDA;
		playerGroup.Rotation.Permissions	= NDA;
		playerGroup.Velocity.Permissions	= NDA;

		SystemRegistration systemReg = {};
		HealthSystem::CreateBaseSystemRegistration(systemReg);

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
		{
			.pSubscriber = &m_LocalPlayerEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() },
			},
			.ComponentGroups = { &playerGroup }
		});

		RegisterSystem(TYPE_NAME(HealthSystemClient), systemReg);
	}

	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &HealthSystemClient::OnPlayerAliveUpdated);
	return true;
}

bool HealthSystemClient::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	using namespace LambdaEngine;

	LOG_INFO("PlayerAliveUpdatedEvent isDead=%s entity=%u",
		event.pPlayer->IsDead() ? "true" : "false",
		event.pPlayer->GetEntity());

	if (event.pPlayer->IsDead())
	{
		PaintMaskRenderer::ResetServer(event.pPlayer->GetEntity());
	}

	return true;
}
