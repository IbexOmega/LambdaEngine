#pragma once
#include "ECS/System.h"

#include "Game/World/Player/PlayerActionSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Game/Multiplayer/GameState.h"

namespace LambdaEngine
{
	class IClient;

	class PlayerSystem
	{
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(PlayerSystem);
		virtual ~PlayerSystem();

		void Init();

		void TickMainThread(Timestamp deltaTime, IClient* pClient);
		void FixedTickMainThread(Timestamp deltaTime, IClient* pClient);

		void TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState);
		void TickOtherPlayersAction(Timestamp deltaTime);

	private:
		PlayerSystem();

		void SendGameState(const GameState& gameState, IClient* pClient);
		bool OnPacketReceived(const PacketReceivedEvent& event);
		void Reconcile();
		void ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer);
		bool CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer);

	private:
		std::unordered_map<Entity, TArray<GameState>> m_EntityOtherStates;
		PlayerActionSystem m_PlayerActionSystem;

		int32 m_NetworkUID;
		int32 m_SimulationTick;
		TArray<GameState> m_FramesToReconcile;
		TArray<GameState> m_FramesProcessedByServer;
	};
}