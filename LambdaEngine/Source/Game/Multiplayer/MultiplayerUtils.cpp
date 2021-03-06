#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/Multiplayer/MultiplayerUtilBase.h"
#include "Game/Multiplayer/Server/ServerUtilsImpl.h"
#include "Game/Multiplayer/Client/ClientUtilsImpl.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkStatistics.h"

namespace LambdaEngine
{
	MultiplayerUtilBase* MultiplayerUtils::s_pMultiplayerUtility = nullptr;
	bool MultiplayerUtils::s_IsServer = false;
	bool MultiplayerUtils::s_IsSinglePlayer = false;

	bool MultiplayerUtils::IsServer()
	{
		return s_IsServer;
	}

	Entity MultiplayerUtils::GetEntity(int32 networkUID)
	{
		return s_pMultiplayerUtility->GetEntity(networkUID);
	}

	int32 MultiplayerUtils::GetNetworkUID(Entity entity)
	{
		return s_pMultiplayerUtility->GetNetworkUID(entity);
	}

	void MultiplayerUtils::RegisterEntity(Entity entity, int32 networkUID)
	{
		s_pMultiplayerUtility->RegisterEntity(entity, networkUID);
	}

	void MultiplayerUtils::UnregisterEntity(Entity entity)
	{
		s_pMultiplayerUtility->UnregisterEntity(entity);
	}

	bool MultiplayerUtils::IsSingleplayer()
	{
		return s_IsSinglePlayer;
	}

	bool MultiplayerUtils::HasWriteAccessToEntity(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		//TODO: Add code checks depending on server or client and ownership of entity
		return true;
	}

	void MultiplayerUtils::SetIsSingleplayer(bool value)
	{
		s_IsSinglePlayer = value;
	}

	void MultiplayerUtils::Init(bool server)
	{
		Release();
		s_IsServer = server;

		if (server)
			s_pMultiplayerUtility = DBG_NEW ServerUtilsImpl();
		else
			s_pMultiplayerUtility = DBG_NEW ClientUtilsImpl();
	}

	void MultiplayerUtils::Release()
	{
		SAFEDELETE(s_pMultiplayerUtility);
	}
}