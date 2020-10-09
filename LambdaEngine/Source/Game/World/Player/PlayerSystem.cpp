#include "Game/World/Player/PlayerSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

#include "Game/Multiplayer/GameState.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/IClient.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#define EPSILON 0.01f

namespace LambdaEngine
{
	PlayerSystem::PlayerSystem() : 
		m_EntityOtherStates(),
		m_PlayerActionSystem(),
		m_FramesToReconcile(),
		m_FramesProcessedByServer(),
		m_SimulationTick(0),
		m_NetworkUID(-1)
	{

	}

	PlayerSystem::~PlayerSystem()
	{

	}

	void PlayerSystem::Init()
	{
		MultiplayerUtils::SubscribeToPacketType(NetworkSegment::TYPE_PLAYER_ACTION, std::bind(&PlayerSystem::OnPacketPlayerAction, this, std::placeholders::_1, std::placeholders::_2));
	}

	void PlayerSystem::FixedTickMainThread(Timestamp deltaTime, IClient* pClient)
	{
		if (m_NetworkUID >= 0)
		{
			Reconcile();

			GameState gameState = {};
			gameState.SimulationTick = m_SimulationTick++;

			Entity entityPlayer = MultiplayerUtils::GetEntity(m_NetworkUID);
			TickLocalPlayerAction(deltaTime, entityPlayer, &gameState);
			TickOtherPlayersAction(deltaTime);
			SendGameState(gameState, pClient);
		}	
	}

	void PlayerSystem::SendGameState(const GameState& gameState, IClient* pClient)
	{
		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
		BinaryEncoder encoder(pPacket);
		encoder.WriteInt32(gameState.SimulationTick);
		encoder.WriteInt8(gameState.DeltaForward);
		encoder.WriteInt8(gameState.DeltaLeft);
		pClient->SendReliable(pPacket);
	}

	void PlayerSystem::TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);
		const PositionComponent& positionComponent = pPositionComponents->GetData(entityPlayer);

		netPosComponent.PositionLast = positionComponent.Position; //Lerpt from the current interpolated position (The rendered one)
		netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();

		m_PlayerActionSystem.DoAction(deltaTime, entityPlayer, pGameState);

		CharacterControllerSystem::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

		pGameState->Position = netPosComponent.Position;
		pGameState->Velocity = velocityComponent.Velocity;

		m_FramesToReconcile.PushBack(*pGameState);
	}

	void PlayerSystem::TickOtherPlayersAction(Timestamp deltaTime)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		for (auto& pair : m_EntityOtherStates)
		{
			GameStateOther& gameState = pair.second;
			Entity entity = pair.first;

			NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entity);
			const PositionComponent& positionComponent = pPositionComponents->GetData(entity);
			VelocityComponent& velocityComponent = pVelocityComponents->GetData(entity);

			if (gameState.HasNewData) //Data does not exist for the current frame :(
			{
				gameState.HasNewData = false;

				netPosComponent.PositionLast = positionComponent.Position;
				netPosComponent.Position = gameState.Position;
				netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();

				velocityComponent.Velocity = gameState.Velocity;
			}
			else //Data exist for the current frame :)
			{
				netPosComponent.PositionLast = positionComponent.Position;
				netPosComponent.Position += velocityComponent.Velocity * dt;
				netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();
			}
		}
	}

	void PlayerSystem::OnPacketPlayerAction(IClient* pClient, NetworkSegment* pPacket)
	{
		GameState serverGameState = {};

		BinaryDecoder decoder(pPacket);
		int32 networkUID = decoder.ReadInt32();
		serverGameState.SimulationTick = decoder.ReadInt32();
		serverGameState.Position = decoder.ReadVec3();
		serverGameState.Velocity = decoder.ReadVec3();

		if (networkUID == m_NetworkUID)
		{
			m_FramesProcessedByServer.PushBack(serverGameState);
		}
		else
		{
			Entity entity = MultiplayerUtils::GetEntity(networkUID);

			m_EntityOtherStates[entity] = {
				serverGameState.Position,
				serverGameState.Velocity,
				true
			};
		}
	}

	void PlayerSystem::Reconcile()
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

	void PlayerSystem::ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity entityPlayer = MultiplayerUtils::GetEntity(m_NetworkUID);

		auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

		netPosComponent.Position = gameStateServer.Position;
		velocityComponent.Velocity = gameStateServer.Velocity;

		//Replay all game states since the game state which resulted in prediction ERROR

		// TODO: Rollback other entities not just the player 

		const Timestamp deltaTime = EngineLoop::GetFixedTimestep();
		const float32 dt = (float32)deltaTime.AsSeconds();

		for (uint32 i = 1; i < count; i++)
		{
			GameState& gameState = pGameStates[i];

			/*
			* Returns the velocity based on key presses
			*/
			PlayerActionSystem::ComputeVelocity(gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);

			/*
			* Sets the position of the PxController taken from the PositionComponent.
			* Move the PxController using the VelocityComponent by the deltatime.
			* Calculates a new Velocity based on the difference of the last position and the new one.
			* Sets the new position of the PositionComponent
			*/
			CharacterControllerSystem::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

			gameState.Position = netPosComponent.Position;
			gameState.Velocity = velocityComponent.Velocity;
		}
	}

	bool PlayerSystem::CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer)
	{
		if (glm::distance(gameStateLocal.Position, gameStateServer.Position) > EPSILON)
		{
			LOG_ERROR("Prediction Error, Tick: %d, Position: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Position.x, gameStateLocal.Position.y, gameStateLocal.Position.z, gameStateServer.Position.x, gameStateServer.Position.y, gameStateServer.Position.z);
			return false;
		}

		if (glm::distance(gameStateLocal.Velocity, gameStateServer.Velocity) > EPSILON)
		{
			LOG_ERROR("Prediction Error, Tick: %d, Velocity: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Velocity.x, gameStateLocal.Velocity.y, gameStateLocal.Velocity.z, gameStateServer.Velocity.x, gameStateServer.Velocity.y, gameStateServer.Velocity.z);
			return false;
		}

		return true;
	}
}