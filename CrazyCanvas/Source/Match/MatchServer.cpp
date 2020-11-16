#include "Match/MatchServer.h"
#include "Match/Match.h"

#include "ECS/ECSCore.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

#include "Math/Random.h"

#include "Networking/API/ClientRemoteBase.h"

#include "Rendering/ImGuiRenderer.h"

#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketMatchStart.h"
#include "Multiplayer/Packet/PacketMatchBegin.h"

#include <imgui.h>

#define RENDER_MATCH_INFORMATION

MatchServer::~MatchServer()
{
	using namespace LambdaEngine;
	
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::UnregisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);
}

void MatchServer::KillPlayer(LambdaEngine::Entity playerEntity)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
	m_PlayersToKill.EmplaceBack(playerEntity);
}

bool MatchServer::InitInternal()
{
	using namespace LambdaEngine;

	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::RegisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);

	return true;
}

void MatchServer::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (!m_HasBegun)
	{
		m_MatchBeginTimer -= float32(deltaTime.AsSeconds());

		if (m_MatchBeginTimer < 0.0f)
		{
			MatchBegin();
		}
	}

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (flagEntities.IsEmpty())
			SpawnFlag();
	}

	//Render Some Server Match Information
#if defined(RENDER_MATCH_INFORMATION)
	ImGuiRenderer::Get().DrawUI([this]()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		if (ImGui::Begin("Match Panel"))
		{
			if (m_pLevel != nullptr)
			{
				// Flags
				TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
				if (!flagEntities.IsEmpty())
				{
					Entity flagEntity = flagEntities[0];

					std::string name = "Flag : [EntityID=" + std::to_string(flagEntity) + "]";
					if (ImGui::TreeNode(name.c_str()))
					{
						const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
						ImGui::Text("Flag Status: %s", flagParentComponent.Attached ? "Carried" : "Not Carried");

						if (flagParentComponent.Attached)
						{
							if (ImGui::Button("Drop Flag"))
							{
								FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, glm::vec3(0.0f, 2.0f, 0.0f));
							}
						}

						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::Text("Flag Status: Not Spawned");
				}

				// Player
				TArray<Entity> playerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);
				if (!playerEntities.IsEmpty())
				{
					ComponentArray<ChildComponent>*		pChildComponents	= pECS->GetComponentArray<ChildComponent>();
					ComponentArray<HealthComponent>*	pHealthComponents	= pECS->GetComponentArray<HealthComponent>();
					ComponentArray<WeaponComponent>*	pWeaponComponents	= pECS->GetComponentArray<WeaponComponent>();

					ImGui::Text("Player Status:");
					for (uint32 i = 0; Entity playerEntity : playerEntities)
					{
						std::string name = "Player " + std::to_string(++i) + " : [EntityID=" + std::to_string(playerEntity) + "]";
						if (ImGui::TreeNode(name.c_str()))
						{
							const HealthComponent& health = pHealthComponents->GetConstData(playerEntity);
							ImGui::Text("Health: %u", health.CurrentHealth);

							const ChildComponent& children = pChildComponents->GetConstData(playerEntity);
							Entity weapon = children.GetEntityWithTag("weapon");

							const WeaponComponent& weaponComp = pWeaponComponents->GetConstData(weapon);

							auto waterAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
							if(waterAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Water Ammunition: %u/%u", waterAmmo->second.first, waterAmmo->second.second);

							auto paintAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
							if (paintAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Paint Ammunition: %u/%u", paintAmmo->second.first, paintAmmo->second.second);


							if (ImGui::Button("Kill"))
							{
								Match::KillPlayer(playerEntity);
							}
							
							ImGui::SameLine();

							if (ImGui::Button("Disconnect"))
							{
								const uint64 uid = m_PlayerEntityToClientID[playerEntity];
								ClientRemoteBase* pClient = ServerHelper::GetClient(uid);
								if (pClient)
								{
									pClient->Disconnect("Kicked");
								}
							}

							ImGui::TreePop();
						}
					}
				}
				else
				{
					ImGui::Text("Player Status: No players");
				}
			}
		}

		ImGui::End();
	});
#endif
}

void MatchServer::MatchStart()
{
	m_HasBegun = false;
	m_MatchBeginTimer = MATCH_BEGIN_COUNTDOWN_TIME;

	PacketMatchStart matchStartPacket;
	ServerHelper::SendBroadcast(matchStartPacket);

	LOG_INFO("SERVER: Match Start");
}

void MatchServer::MatchBegin()
{
	m_HasBegun = true;

	PacketMatchBegin matchBeginPacket;
	ServerHelper::SendBroadcast(matchBeginPacket);

	LOG_INFO("SERVER: Match Begin");
}

void MatchServer::FixedTickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(deltaTime);

	{
		std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
		for (Entity playerEntity : m_PlayersToKill)
		{
			LOG_INFO("SERVER: Player=%u DIED", playerEntity);
			KillPlayerInternal(playerEntity);
		}

		m_PlayersToKill.Clear();
	}
}

void MatchServer::SpawnFlag()
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

	if (!flagSpawnPointEntities.IsEmpty())
	{
		Entity flagSpawnPoint = flagSpawnPointEntities[0];
		const FlagSpawnComponent& flagSpawnComponent	= pECS->GetConstComponent<FlagSpawnComponent>(flagSpawnPoint);
		const PositionComponent& positionComponent		= pECS->GetConstComponent<PositionComponent>(flagSpawnPoint);

		float r		= Random::Float32(0.0f, flagSpawnComponent.Radius);
		float theta = Random::Float32(0.0f, glm::two_pi<float32>());

		CreateFlagDesc createDesc = {};
		createDesc.Position		= positionComponent.Position + r * glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta));
		createDesc.Scale		= glm::vec3(1.0f);
		createDesc.Rotation		= glm::identity<glm::quat>();

		TArray<Entity> createdFlagEntities;
		if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createDesc, createdFlagEntities))
		{
			VALIDATE(createdFlagEntities.GetSize() == 1);

			PacketCreateLevelObject packet;
			packet.LevelObjectType			= ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;
			packet.Position					= createDesc.Position;
			packet.Forward					= GetForward(createDesc.Rotation);
			packet.Flag.ParentNetworkUID	= INT32_MAX;

			//Tell the bois that we created a flag
			for (Entity entity : createdFlagEntities)
			{
				packet.NetworkUID = entity;

				ServerHelper::SendBroadcast(packet);
			}
		}
		else
		{
			LOG_ERROR("[MatchServer]: Failed to create Flag");
		}
	}
}

bool MatchServer::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FireDirection	= event.Direction;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity	= event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;
	createProjectileDesc.MeshComponent	= event.MeshComponent;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchServer]: Failed to create projectile!");
	}

	LOG_INFO("SERVER: Weapon fired");
	return false;
}

void MatchServer::SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> playerSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	glm::vec3 position(0.0f, 5.0f, 0.0f);
	glm::vec3 forward(0.0f, 0.0f, 1.0f);

	for (Entity spawnPoint : playerSpawnPointEntities)
	{
		const TeamComponent& teamComponent = pTeamComponents->GetConstData(spawnPoint);

		if (teamComponent.TeamIndex == m_NextTeamIndex)
		{
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(spawnPoint);
			position = positionComponent.Position + glm::vec3(0.0f, 1.0f, 0.0f);
			forward = glm::normalize(-glm::vec3(position.x, 0.0f, position.z));
			break;
		}
	}

	CreatePlayerDesc createPlayerDesc =
	{
		.Position		= position,
		.Forward		= forward,
		.Scale			= glm::vec3(1.0f),
		.TeamIndex		= m_NextTeamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position			= position;
		packet.Forward			= forward;
		packet.Player.TeamIndex	= m_NextTeamIndex;

		ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();
		for (Entity playerEntity : createdPlayerEntities)
		{
			const ChildComponent& childComp = pCreatedChildComponents->GetConstData(playerEntity);
			packet.Player.IsMySelf	= true;
			packet.NetworkUID		= playerEntity;
			packet.Player.WeaponNetworkUID = childComp.GetEntityWithTag("weapon");

			ServerHelper::Send(pClient, packet);

			packet.Player.IsMySelf	= false;
			ServerHelper::SendBroadcast(packet, nullptr, pClient);
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
	}

	const uint64 cliendID = pClient->GetUID();
	if (m_ClientIDToPlayerEntity.count(cliendID) == 0)
	{
		m_ClientIDToPlayerEntity.insert(std::make_pair(cliendID, createdPlayerEntities[0]));
		m_PlayerEntityToClientID.insert(std::make_pair(createdPlayerEntities[0], cliendID));
	}

	m_NextTeamIndex = (m_NextTeamIndex + 1) % 2;
}

void MatchServer::DeleteGameLevelObject(LambdaEngine::Entity entity)
{
	m_pLevel->DeleteObject(entity);

	PacketDeleteLevelObject packet;
	packet.NetworkUID = entity;

	ServerHelper::SendBroadcast(packet);
}

bool MatchServer::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	IClient* pClient = event.pClient;

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
	ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
	ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();

	// Send currently existing players to the new client
	{
		TArray<Entity> playerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Player.IsMySelf	= false;

		for (Entity otherPlayerEntity : playerEntities)
		{
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(otherPlayerEntity);
			const RotationComponent& rotationComponent = pRotationComponents->GetConstData(otherPlayerEntity);
			const TeamComponent& teamComponent	= pTeamComponents->GetConstData(otherPlayerEntity);
			const ChildComponent& childComp		= pCreatedChildComponents->GetConstData(otherPlayerEntity);

			packet.NetworkUID				= otherPlayerEntity;
			packet.Player.WeaponNetworkUID	= childComp.GetEntityWithTag("weapon");
			packet.Position			= positionComponent.Position;
			packet.Forward			= GetForward(rotationComponent.Quaternion);
			packet.Player.TeamIndex	= teamComponent.TeamIndex;
			ServerHelper::Send(pClient, packet);
		}
	}

	// Create a player for the new client, also sends the new player to the connected clients
	{
		SpawnPlayer((ClientRemoteBase*)pClient);
	}

	// Send flag data to clients
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		PacketCreateLevelObject packet;
		packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;

		for (Entity flagEntity : flagEntities)
		{
			const PositionComponent& positionComponent	= pPositionComponents->GetConstData(flagEntity);
			const RotationComponent& rotationComponent	= pRotationComponents->GetConstData(flagEntity);
			const ParentComponent& parentComponent		= pParentComponents->GetConstData(flagEntity);

			packet.NetworkUID				= flagEntity;
			packet.Position					= positionComponent.Position;
			packet.Forward					= GetForward(rotationComponent.Quaternion);
			packet.Flag.ParentNetworkUID	= parentComponent.Parent;
			ServerHelper::Send(pClient, packet);
		}
	}

	// Match Start
	{
		MatchStart();
	}

	return true;
}

bool MatchServer::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	VALIDATE(event.pClient != nullptr);

	const uint64 clientID = event.pClient->GetUID();
	const LambdaEngine::Entity playerEntity = m_ClientIDToPlayerEntity[clientID];
	m_ClientIDToPlayerEntity.erase(clientID);
	m_PlayerEntityToClientID.erase(playerEntity);

	Match::KillPlayer(playerEntity);

	// TODO: Fix this
	//DeleteGameLevelObject(playerEntity);

	return true;
}

bool MatchServer::OnFlagDelivered(const FlagDeliveredEvent& event)
{
	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (!flagEntities.IsEmpty())
		{
			Entity flagEntity = flagEntities[0];
			FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, glm::vec3(0.0f, 2.0f, 0.0f));
		}
	}

	uint32 newScore = GetScore(event.TeamIndex) + 1;
	SetScore(event.TeamIndex, newScore);


	PacketTeamScored packet;
	packet.TeamIndex	= event.TeamIndex;
	packet.Score		= newScore;
	ServerHelper::SendBroadcast(packet);

	if (newScore == m_MatchDesc.MaxScore) // game over
	{
		PacketGameOver gameOverPacket;
		gameOverPacket.WinningTeamIndex = event.TeamIndex;

		ServerHelper::SendBroadcast(gameOverPacket);

		ResetMatch();
	}

	return true;
}

void MatchServer::KillPlayerInternal(LambdaEngine::Entity playerEntity)
{
	using namespace LambdaEngine;

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<NetworkPositionComponent>* pNetworkPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	if (pNetworkPosComponents != nullptr && pNetworkPosComponents->HasComponent(playerEntity))
	{
		NetworkPositionComponent& positionComp = pECS->GetComponent<NetworkPositionComponent>(playerEntity);

		// Get spawnpoint from level
		const glm::vec3 oldPosition = positionComp.Position;
		glm::vec3 newPosition = glm::vec3(0.0f);
		if (m_pLevel != nullptr)
		{
			// Retrive spawnpoints
			TArray<Entity> spawnPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

			ComponentArray<PositionComponent>*	pPositionComponents = pECS->GetComponentArray<PositionComponent>();
			ComponentArray<TeamComponent>*		pTeamComponents		= pECS->GetComponentArray<TeamComponent>();

			uint8 playerTeam = pTeamComponents->GetConstData(playerEntity).TeamIndex;
			for (Entity spawnEntity : spawnPoints)
			{
				if (pTeamComponents->HasComponent(spawnEntity))
				{
					if (pTeamComponents->GetConstData(spawnEntity).TeamIndex == playerTeam)
					{
						newPosition = pPositionComponents->GetConstData(spawnEntity).Position;
					}
				}
			}

			// Drop flag if player carries it
			TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
			if (!flagEntities.IsEmpty())
			{
				Entity flagEntity = flagEntities[0];

				const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
				if (flagParentComponent.Attached && flagParentComponent.Parent == playerEntity)
				{
					FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, oldPosition);
				}
			}
		}

		// Reset position
		positionComp.Position = newPosition;
	}
	else
	{
		LOG_WARNING("Killed player called for entity(=%u) that does not have a NetworkPositionComponent", playerEntity);
	}
}
