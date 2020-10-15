#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Server/ClientRemoteSystem.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Engine/EngineConfig.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	ServerSystem* ServerSystem::s_pInstance = nullptr;

	ServerSystem::ServerSystem(const String& name) :
		m_NetworkEntities(),
		m_pServer(nullptr),
		m_CharacterControllerSystem(),
		m_Name(name)
	{
		MultiplayerUtils::Init(true);

		ServerDesc desc = {};
		desc.Handler				= this;
		desc.MaxRetries				= 10;
		desc.ResendRTTMultiplier	= 5.0f;
		desc.MaxClients				= 10;
		desc.PoolSize				= 1024;
		desc.Protocol				= EProtocol::UDP;
		desc.PingInterval			= Timestamp::Seconds(1);
		desc.PingTimeout			= Timestamp::Seconds(10);
		desc.UsePingSystem			= true;

		m_pServer = NetworkUtils::CreateServer(desc);
		//((ServerUDP*)m_pServer)->SetSimulateReceivingPacketLoss(0.1f);

		m_CharacterControllerSystem.Init();
	}

	ServerSystem::~ServerSystem()
	{
		m_pServer->Release();
		MultiplayerUtils::Release();
	}

	bool ServerSystem::Start()
	{
		uint16 port = (uint16)EngineConfig::GetIntProperty("NetworkPort");
		NetworkDiscovery::EnableServer(m_Name, port, this);
		return m_pServer->Start(IPEndPoint(IPAddress::ANY, port));
	}

	void ServerSystem::Stop()
	{
		NetworkDiscovery::DisableServer();
		m_pServer->Stop("ServerSystem->Stop()");
	}

	void ServerSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		const ClientMap& pClients = m_pServer->GetClients();
		for (auto& pair : pClients)
		{
			ClientRemoteSystem* pClientSystem = (ClientRemoteSystem*)pair.second->GetHandler();
			pClientSystem->FixedTickMainThread(deltaTime);
		}
	}

	void ServerSystem::TickMainThread(Timestamp deltaTime)
	{
		NetworkDebugger::RenderStatistics(m_pServer);

		const ClientMap& pClients = m_pServer->GetClients();
		for (auto& pair : pClients)
		{
			ClientRemoteSystem* pClientSystem = (ClientRemoteSystem*)pair.second->GetHandler();
			pClientSystem->TickMainThread(deltaTime);
		}
	}

	IClientRemoteHandler* ServerSystem::CreateClientHandler()
	{
		return DBG_NEW ClientRemoteSystem();
	}

	void ServerSystem::OnNetworkDiscoveryPreTransmit(BinaryEncoder& encoder)
	{
		ServerDiscoveryPreTransmitEvent event(&encoder, m_pServer);
		EventQueue::SendEventImmediate(event);
	}

	void ServerSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
	}

	void ServerSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ServerSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}