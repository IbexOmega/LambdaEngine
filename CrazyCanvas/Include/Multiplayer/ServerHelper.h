#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Networking/API/IPacketListener.h"

#include "Lobby/Player.h"
#include "Lobby/PlayerManagerServer.h"

#define VALIDATE_PACKET_TYPE(T) ASSERT_MSG(T::GetType() != 0, "Packet type [%s] not registered!", #T)

class ServerHelper
{
	DECL_STATIC_CLASS(ServerHelper);

public:
	template<class T>
	static bool Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	template<class T>
	static bool SendToPlayer(const Player* pPlayer, const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	template<class T>
	static bool SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr, LambdaEngine::IClient* pExcludeClient = nullptr);

	template<class T>
	static bool SendToAllPlayers(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr, const Player* pPlayer = nullptr);

	// Sends a package to all teammates of the given player, except the player himself
	template<class T>
	static bool SendToAllTeammatesOfPlayer(const Player* pExcludePlayer, const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	static void SetMaxClients(uint8 clients);
	static void SetIgnoreNewClients(bool ignore);

	static void DisconnectPlayer(const Player* pPlayer, const LambdaEngine::String& reason);

	static void SetTimeout(LambdaEngine::Timestamp time);
	static void ResetTimeout();
};

template<class T>
bool ServerHelper::Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener)
{
	VALIDATE_PACKET_TYPE(T)
	return pClient->SendReliableStruct<T>(packet, T::GetType(), pListener);
}

template<class T>
bool ServerHelper::SendToPlayer(const Player* pPlayer, const T& packet, LambdaEngine::IPacketListener* pListener)
{
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();
	LambdaEngine::ClientRemoteBase* pClient = pServer->GetClient(pPlayer->GetUID());
	return pClient != nullptr ? Send<T>(pClient, packet, pListener) : false;
}

template<class T>
bool ServerHelper::SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener, LambdaEngine::IClient* pExcludeClient)
{
	VALIDATE_PACKET_TYPE(T)
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();
	return pServer->SendReliableStructBroadcast<T>(packet, T::GetType(), pListener, pExcludeClient);
}

template<class T>
bool ServerHelper::SendToAllPlayers(const T& packet, LambdaEngine::IPacketListener* pListener, const Player* pExcludePlayer)
{
	VALIDATE_PACKET_TYPE(T)
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();
	return pServer->SendReliableStructBroadcast<T>(packet, T::GetType(), pListener, pServer->GetClient(pExcludePlayer->GetUID()));
}

template<class T>
bool ServerHelper::SendToAllTeammatesOfPlayer(const Player* pExcludePlayer, const T& packet, LambdaEngine::IPacketListener* pListener)
{
	VALIDATE_PACKET_TYPE(T)
	LambdaEngine::TArray<const Player*> players;
	PlayerManagerServer::GetPlayersOfTeam(players, pExcludePlayer->GetTeam());
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();

	bool success = true;
	for (const Player* pPlayer : players)
	{
		if (pPlayer != pExcludePlayer)
		{
			LambdaEngine::ClientRemoteBase* pClient = pServer->GetClient(pPlayer->GetUID());
			if (pClient)
			{
				if (!Send<T>(pClient, packet, pListener))
				{
					success = false;
				}
			}
		}
	}

	return success;
}
