#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	std::set<ServerUDP*> ServerUDP::s_Servers;
	SpinLock ServerUDP::s_Lock;

	ServerUDP::ServerUDP(const ServerUDPDesc& desc) :
		m_Desc(desc),
		m_pSocket(nullptr),
		m_Accepting(true),
		m_PacketLoss(0.0f)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.insert(this);
	}

	ServerUDP::~ServerUDP()
	{
		for (auto& pair : m_Clients)
		{
			delete pair.second;
		}
		m_Clients.clear();

		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.erase(this);

		LOG_INFO("[ServerUDP]: Released");
	}

	bool ServerUDP::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerUDP]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerUDP::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerUDP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ServerUDP::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerUDP::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerUDP::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	void ServerUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ServerUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	bool ServerUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				m_Transciver.SetSocket(m_pSocket);
				LOG_INFO("[ServerUDP]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
		}
		return false;
	}

	void ServerUDP::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			bool newConnection = false;
			ClientUDPRemote* pClient = GetOrCreateClient(sender, newConnection);

			if (newConnection)
			{
				if (!IsAcceptingConnections())
				{
					SendServerNotAccepting(pClient);
					pClient->Release();
					continue;
				}
				else if (m_Clients.size() >= m_Desc.MaxClients)
				{
					SendServerFull(pClient);
					pClient->Release();
					continue;
				}
				else
				{
					m_Clients.insert({ sender, pClient });
				}
			}

			pClient->OnDataReceived(&m_Transciver);
		}
	}

	void ServerUDP::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& tuple : m_Clients)
				{
					tuple.second->SendPackets(&m_Transciver);
				}
			}
		}
	}
	
	void ServerUDP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerUDP]: Stopped");
	}

	void ServerUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ServerUDP]: Stopping...");
	}

	void ServerUDP::OnReleaseRequested()
	{
		Stop();
	}

	IClientRemoteHandler* ServerUDP::CreateClientHandler()
	{
		return m_Desc.Handler->CreateClientHandler();
	}

	ClientUDPRemote* ServerUDP::GetOrCreateClient(const IPEndPoint& sender, bool& newConnection)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		auto pIterator = m_Clients.find(sender);
		if (pIterator != m_Clients.end())
		{
			newConnection = false;
			return pIterator->second;
		}
		else
		{
			newConnection = true;
			return DBG_NEW ClientUDPRemote(m_Desc.PoolSize, m_Desc.MaxRetries, sender, this);
		}
	}

	void ServerUDP::OnClientDisconnected(ClientUDPRemote* client, bool sendDisconnectPacket)
	{
		if(sendDisconnectPacket)
			SendDisconnect(client);

		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.erase(client->GetEndPoint());
	}

	void ServerUDP::SendDisconnect(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_DISCONNECT));
		client->SendPackets(&m_Transciver);
	}

	void ServerUDP::SendServerFull(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_FULL));
		client->SendPackets(&m_Transciver);
	}

	void ServerUDP::SendServerNotAccepting(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING));
		client->SendPackets(&m_Transciver);
	}

	void ServerUDP::Tick(Timestamp delta)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		for (auto& pair : m_Clients)
		{
			pair.second->Tick(delta);
		}
		Flush();
	}

	ServerUDP* ServerUDP::Create(const ServerUDPDesc& desc)
	{
		return DBG_NEW ServerUDP(desc);
	}

	void ServerUDP::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_Servers.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ServerUDP* server : s_Servers)
			{
				server->Tick(timestamp);
			}
		}
	}
}