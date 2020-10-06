#include "Engine/EngineLoop.h"

#include "Game/ECS/Systems/Networking/ClientSystem.h"
#include "Game/ECS/Systems/Networking/InterpolationSystem.h"
#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Components/Networking/InterpolationComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"

#include "Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#define EPSILON 0.01f

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem() :
		m_ControllableEntities(),
		m_pClient(nullptr),
		m_FramesToReconcile(),
		m_FramesProcessedByServer(),
		m_SimulationTick(0),
		m_LastNetworkSimulationTick(0),
		m_Entities(),
		m_NetworkUID(-1),
		m_CharacterControllerSystem(),
		m_pInterpolationSystem(nullptr)
	{

	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
	}

	void ClientSystem::Init()
	{
		ClientDesc clientDesc			= {};
		clientDesc.PoolSize				= 1024;
		clientDesc.MaxRetries			= 10;
		clientDesc.ResendRTTMultiplier	= 5.0F;
		clientDesc.Handler				= this;
		clientDesc.Protocol				= EProtocol::UDP;
		clientDesc.PingInterval			= Timestamp::Seconds(1);
		clientDesc.PingTimeout			= Timestamp::Seconds(3);
		clientDesc.UsePingSystem		= true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);


		SubscribeToPacketType(NetworkSegment::TYPE_ENTITY_CREATE, std::bind(&ClientSystem::OnPacketCreateEntity, this, std::placeholders::_1));
		SubscribeToPacketType(NetworkSegment::TYPE_PLAYER_ACTION, std::bind(&ClientSystem::OnPacketPlayerAction, this, std::placeholders::_1));

		m_pInterpolationSystem = DBG_NEW InterpolationSystem();
		m_pInterpolationSystem->Init();

		m_CharacterControllerSystem.Init();
	}

	bool ClientSystem::Connect(IPAddress* pAddress)
	{
		if (!m_pClient->Connect(IPEndPoint(pAddress, 4444)))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::SubscribeToPacketType(uint16 packetType, const std::function<void(NetworkSegment*)>& func)
	{
		m_PacketSubscribers[packetType].PushBack(func);
	}

	Entity ClientSystem::GetEntityFromNetworkUID(int32 networkUID) const
	{
		auto pair = m_Entities.find(networkUID);
		ASSERT(pair != m_Entities.end());
		return pair->second;
	}

	bool ClientSystem::IsLocalClient(int32 networkUID) const
	{
		return m_NetworkUID == networkUID;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		using namespace physx;
		UNREFERENCED_VARIABLE(deltaTime);

		if (m_pClient->IsConnected() && m_Entities.size() > 0)
		{
			ECSCore* pECS = ECSCore::GetInstance();
			Entity entityPlayer = GetEntityPlayer();
			float32 dt = deltaTime.AsSeconds();

			int8 deltaForward	= int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
			int8 deltaLeft		= int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

			auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
			auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
			auto* pCharacterLocalColliderComponents = pECS->GetComponentArray<CharacterLocalColliderComponent>();
			auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

			const PositionComponent& positionComponent = pPositionComponents->GetData(entityPlayer);
			CharacterColliderComponent& characterColliderComponent = pCharacterColliderComponents->GetData(entityPlayer);
			CharacterLocalColliderComponent& characterLocalColliderComponent = pCharacterLocalColliderComponents->GetData(entityPlayer);
			VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);



			PlayerMovementSystem::GetInstance().PredictVelocity(deltaForward, deltaLeft, velocityComponent.Velocity);



			const glm::vec3& position = positionComponent.Position;
			const glm::vec3& velocity = velocityComponent.Velocity;

			PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
			translationPX *= dt;

			PxController* pController = characterLocalColliderComponent.pController;

			pController->setPosition(characterColliderComponent.pController->getPosition());

			/*LOG_WARNING("POS: %f, %f, %f", pController->getPosition().x, pController->getPosition().y, pController->getPosition().z);
			LOG_WARNING("VEL: %f, %f, %f", translationPX.x, translationPX.y, translationPX.z);*/

			pController->move(translationPX, 0.0f, dt, characterLocalColliderComponent.Filters);

			const PxExtendedVec3& positionPX = pController->getPosition();
			const glm::vec3 positionPredicted((float32)positionPX.x, (float32)positionPX.y, (float32)positionPX.z);
			const glm::vec3 velocityPredicted = (positionPredicted - position) / dt;

			/*LOG_WARNING("POS2: %f, %f, %f", positionPredicted.x, positionPredicted.y, positionPredicted.z);
			LOG_WARNING("VEL2: %f, %f, %f", velocityPredicted.x, velocityPredicted.y, velocityPredicted.z);*/

			LOG_WARNING("C GameState %d: %f, %f, %f, %f, %f, %f | %d, %d", m_SimulationTick, positionPredicted.x, positionPredicted.y, positionPredicted.z, position.x, position.y, position.z, deltaForward, deltaLeft);
			if (velocityPredicted.x != 0 || velocityPredicted.z != 0)
			{
				//LOG_INFO("Prediction: [Vel: %f, %f, %f] [VelP: %f, %f, %f] [Pos: %f, %f, %f] [Tick: %d]", velocity.x, velocity.y, velocity.z, velocityPredicted.x, velocityPredicted.y, velocityPredicted.z, positionPredicted.x, positionPredicted.y, positionPredicted.z, m_SimulationTick);
			}



			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
			BinaryEncoder encoder(pPacket);
			encoder.WriteInt32(m_SimulationTick);
			encoder.WriteInt8(deltaForward);
			encoder.WriteInt8(deltaLeft);
			m_pClient->SendReliable(pPacket);

			GameState gameState			= {};
			gameState.SimulationTick	= m_SimulationTick;
			gameState.DeltaForward		= deltaForward;
			gameState.DeltaLeft			= deltaLeft;
			gameState.Position			= positionPredicted;
			gameState.Velocity			= velocityPredicted;

			m_FramesToReconcile.PushBack(gameState);
			Reconcile();

			m_SimulationTick++;
		}
	}

	Entity ClientSystem::GetEntityPlayer() const
	{
		return GetEntityFromNetworkUID(m_NetworkUID);
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		NetworkDebugger::RenderStatistics(m_pClient);
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnConnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		UNREFERENCED_VARIABLE(pClient);

		auto iterator = m_PacketSubscribers.find(pPacket->GetType());
		if (iterator != m_PacketSubscribers.end())
		{
			const TArray<std::function<void(NetworkSegment*)>>& functions = iterator->second;
			for (const auto& func : functions)
			{
				func(pPacket);
			}
		}
		else
		{
			LOG_WARNING("No packet subscription of type: %hu", pPacket->GetType());
		}
	}

	void ClientSystem::OnPacketCreateEntity(NetworkSegment* pPacket)
	{
		BinaryDecoder decoder(pPacket);
		bool isMySelf		= decoder.ReadBool();
		int32 networkUID	= decoder.ReadInt32();
		glm::vec3 position	= decoder.ReadVec3();
		glm::vec3 color		= decoder.ReadVec3();

		if (isMySelf)
			m_NetworkUID = networkUID;

		Job addEntityJob;
		addEntityJob.Components =
		{
			{ RW, PositionComponent::Type()					},
			{ RW, RotationComponent::Type()					},
			{ RW, ScaleComponent::Type()					},
			{ RW, MeshComponent::Type()						},
			{ RW, NetworkComponent::Type()					},
			{ RW, VelocityComponent::Type()					},
			{ RW, CharacterColliderComponent::Type()		},
			{ RW, CharacterLocalColliderComponent::Type()	}
		};
		addEntityJob.Function = std::bind(&ClientSystem::CreateEntity, this, networkUID, position, color);

		ECSCore::GetInstance()->ScheduleJobASAP(addEntityJob);
	}

	void ClientSystem::OnPacketPlayerAction(NetworkSegment* pPacket)
	{
		GameState serverGameState = {};

		BinaryDecoder decoder(pPacket);
		int32 networkUID				= decoder.ReadInt32();
		serverGameState.SimulationTick	= decoder.ReadInt32();
		serverGameState.Position		= decoder.ReadVec3();
		serverGameState.Velocity		= decoder.ReadVec3();

		if (IsLocalClient(networkUID))
		{
			m_FramesProcessedByServer.PushBack(serverGameState);
		}
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::CreateEntity(int32 networkUID, const glm::vec3& position, const glm::vec3& color)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		LOG_INFO("Creating Entity with ID %d and NetworkID %d", entity, networkUID);

		MaterialProperties materialProperties = {};
		materialProperties.Roughness	= 0.1f;
		materialProperties.Metallic		= 0.0f;
		materialProperties.Albedo		= glm::vec4(color, 1.0f);

		//TArray<GUID_Lambda> animations;

		//const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		//const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MeshComponent meshComponent;
		meshComponent.MeshGUID		= ResourceManager::LoadMeshFromFile("Sphere.obj");
		meshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Mirror Material" + std::to_string(entity),
			//robotAlbedoGUID,
			//robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		//AnimationComponent animationComp;
		//animationComp.AnimationGUID = animations[0];


		pECS->AddComponent<PositionComponent>(entity,	{ true, position });
		pECS->AddComponent<RotationComponent>(entity,	{ true, glm::identity<glm::quat>() });
		pECS->AddComponent<ScaleComponent>(entity,		{ true, glm::vec3(1.0f) });
		pECS->AddComponent<VelocityComponent>(entity,	{ true, glm::vec3(0.0f) });
		//pECS->AddComponent<AnimationComponent>(entity,	animationComp);
		pECS->AddComponent<MeshComponent>(entity,		meshComponent);
		pECS->AddComponent<NetworkComponent>(entity,	{ networkUID });

		const CharacterColliderInfo colliderInfo = {
			.Entity = entity,
			.Position = pECS->GetComponent<PositionComponent>(entity),
			.Rotation = pECS->GetComponent<RotationComponent>(entity),
			.CollisionGroup = FCollisionGroup::COLLISION_GROUP_PLAYER,
			.CollisionMask = FCollisionGroup::COLLISION_GROUP_STATIC | FCollisionGroup::COLLISION_GROUP_PLAYER
		};

		constexpr const float capsuleHeight = 1.8f;
		constexpr const float capsuleRadius = 0.2f;
		CharacterColliderComponent characterColliderComponent;
		PhysicsSystem::GetInstance()->CreateCharacterCapsule(colliderInfo, std::max(0.0f, capsuleHeight - 2.0f * capsuleRadius), capsuleRadius, characterColliderComponent);
		pECS->AddComponent<CharacterColliderComponent>(entity, characterColliderComponent);


		m_Entities.insert({ networkUID, entity });

		if (IsLocalClient(networkUID))
		{
			pECS->AddComponent<ControllableComponent>(entity, { true });


			CharacterLocalColliderComponent characterLocalColliderComponent;
			PhysicsSystem::GetInstance()->CreateCharacterCapsule(colliderInfo, std::max(0.0f, capsuleHeight - 2.0f * capsuleRadius), capsuleRadius, characterLocalColliderComponent);
			pECS->AddComponent<CharacterLocalColliderComponent>(entity, characterLocalColliderComponent);
		}
		else
			pECS->AddComponent<InterpolationComponent>(entity, { glm::vec3(0.0f), glm::vec3(0.0f), 0 });
	}

	void ClientSystem::Reconcile()
	{
		while (!m_FramesProcessedByServer.IsEmpty())
		{
			ASSERT(m_FramesProcessedByServer[0].SimulationTick == m_FramesToReconcile[0].SimulationTick);

			if (!CompareGameStates(m_FramesToReconcile[0], m_FramesProcessedByServer[0]))
			{
				ReplayGameStatesBasedOnServerGameState(m_FramesToReconcile.GetData(), m_FramesToReconcile.GetSize(), m_FramesProcessedByServer[0]);
			}

			m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
			m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.Begin());
		}
	}

	void ClientSystem::ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity entityPlayer = GetEntityPlayer();

		auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		CharacterColliderComponent& characterColliderComponent = pCharacterColliderComponents->GetData(entityPlayer);
		PositionComponent& positionComponent = pPositionComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

		positionComponent.Position = gameStateServer.Position;
		velocityComponent.Velocity = gameStateServer.Velocity;

		//Replay all game states since the game state which resulted in prediction ERROR

		// TODO: Rollback other entities not just the player 

		const Timestamp deltaTime = EngineLoop::GetFixedTimestep();
		const float32 dt = deltaTime.AsSeconds();

		for (uint32 i = 1; i < count; i++)
		{
			GameState& gameState = pGameStates[i];

			/*
			* Returns the velocity based on key presses
			*/
			PlayerMovementSystem::GetInstance().PredictVelocity(gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);

			/*
			* Sets the position of the PxController taken from the PositionComponent.
			* Move the PxController using the VelocityComponent by the deltatime.
			* Calculates a new Velocity based on the difference of the last position and the new one.
			*/
			CharacterControllerSystem::TickCharacterController(dt, entityPlayer, characterColliderComponent, positionComponent, velocityComponent);
			positionComponent.Position += velocityComponent.Velocity * dt;

			gameState.Position = positionComponent.Position;
			gameState.Velocity = velocityComponent.Velocity;


			LOG_MESSAGE("U GameState %d: %f, %f, %f | %d, %d", gameState.SimulationTick, gameState.Position.x, gameState.Position.y, gameState.Position.z, gameState.DeltaForward, gameState.DeltaLeft);

			//PlayerUpdate(entityPlayer, pGameStates[i]);
		}

		const GameState& nextToLastGameState = m_FramesToReconcile[m_FramesToReconcile.GetSize() - 2];

		positionComponent.Position = nextToLastGameState.Position;
		//velocityComponent.Velocity = nextToLastGameState.Velocity;
		

		const GameState& g = m_FramesToReconcile[m_FramesToReconcile.GetSize() - 1];
		LOG_MESSAGE("R GameState %d: %f, %f, %f | %d, %d", g.SimulationTick, g.Position.x, g.Position.y, g.Position.z, g.DeltaForward, g.DeltaLeft);
	}

	bool ClientSystem::CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer)
	{
		if (glm::distance(gameStateLocal.Position, gameStateServer.Position) > EPSILON)
		{
			LOG_ERROR("E GameState %d: [L: %f, %f, %f] [S: %f, %f, %f]  | %d, %d", gameStateLocal.SimulationTick, gameStateLocal.Position.x, gameStateLocal.Position.y, gameStateLocal.Position.z, gameStateServer.Position.x, gameStateServer.Position.y, gameStateServer.Position.z, gameStateLocal.DeltaForward, gameStateLocal.DeltaLeft);
			return false;
		}
			
		if (glm::distance(gameStateLocal.Velocity, gameStateServer.Velocity) > EPSILON)
			return false;

		LOG_INFO("G GameState %d: [L: %f, %f, %f] [S: %f, %f, %f]  | %d, %d", gameStateLocal.SimulationTick, gameStateLocal.Position.x, gameStateLocal.Position.y, gameStateLocal.Position.z, gameStateServer.Position.x, gameStateServer.Position.y, gameStateServer.Position.z, gameStateLocal.DeltaForward, gameStateLocal.DeltaLeft);
		return true;
	}

	void ClientSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
	}

	void ClientSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ClientSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}