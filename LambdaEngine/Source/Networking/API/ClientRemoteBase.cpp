#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/NetworkChallenge.h"
#include "Networking/API/ServerBase.h"

#include "Engine/EngineLoop.h"

#include "Rendering/ImGuiRenderer.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

namespace LambdaEngine
{
	SpinLock ClientRemoteBase::s_LockStatic;
	std::atomic_int ClientRemoteBase::s_Instances = 0;
	THashTable<ClientRemoteBase*, uint8> ClientRemoteBase::s_ClientRemoteBasesToDelete;

	ClientRemoteBase::ClientRemoteBase(const ClientRemoteDesc& desc) :
		m_pServer(desc.Server),
		m_PingInterval(desc.PingInterval),
		m_PingTimeoutDefault(desc.PingTimeout),
		m_UsePingSystem(desc.UsePingSystem),
		m_PingTimeout(desc.PingTimeout),
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_LastPingTimestamp(0),
		m_DisconnectedByRemote(false),
		m_TerminationRequested(false),
		m_TerminationApproved(false),
		m_BufferIndex(0),
		m_ReceivedPackets(),
		m_LockReceivedPackets()
	{
		s_Instances++;
	}

	ClientRemoteBase::~ClientRemoteBase()
	{
		if (!m_TerminationApproved)
			LOG_ERROR("[ClientRemoteBase]: Do not use delete on a ClientRemoteBase object. Use the Release() function!");
		else
			LOG_INFO("[ClientRemoteBase]: Released");

		s_Instances--;
	}

	void ClientRemoteBase::Disconnect(const std::string& reason)
	{
		RequestTermination(reason);
	}

	bool ClientRemoteBase::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	void ClientRemoteBase::Release()
	{
		RequestTermination("Release Requested");
	}

	void ClientRemoteBase::ReleaseByServer()
	{
		RequestTermination("Release Requested By Server", true);
		OnTerminationApproved();
	}

	bool ClientRemoteBase::RequestTermination(const std::string& reason, bool byServer)
	{
		bool doRelease = false;
		bool enterCritical = false;
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			if (!m_TerminationRequested)
			{
				m_TerminationRequested = true;
				doRelease = true;
				enterCritical = OnTerminationRequested();
			}
		}

		if (doRelease)
		{
			if (enterCritical)
			{
				LOG_WARNING("[ClientRemoteBase]: Disconnecting... [%s]", reason.c_str());
				if (m_pHandler)
					m_pHandler->OnDisconnecting(this, reason);

				if (!m_DisconnectedByRemote)
					SendDisconnect();

				if (!byServer)
					m_pServer->OnClientAskForTermination(this);

				m_State = STATE_DISCONNECTED;

				LOG_INFO("[ClientRemoteBase]: Disconnected");
				if (m_pHandler)
					m_pHandler->OnDisconnected(this, reason);

				return true;
			}
		}
		return false;
	}

	bool ClientRemoteBase::OnTerminationRequested()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			m_State = STATE_DISCONNECTING;
			return true;
		}
		return false;
	}

	void ClientRemoteBase::OnTerminationApproved()
	{
		m_TerminationApproved = true;
		DeleteThis();
	}

	void ClientRemoteBase::DeleteThis()
	{
		if (CanDeleteNow())
		{
			if (m_pHandler)
				m_pHandler->OnClientReleased(this);

			std::scoped_lock<SpinLock> lock(s_LockStatic);
			s_ClientRemoteBasesToDelete.insert(std::make_pair(this, (uint8)5));
		}
	}

	bool ClientRemoteBase::CanDeleteNow()
	{
		return m_TerminationApproved;
	}

	void ClientRemoteBase::OnConnectionApproved()
	{
		m_pHandler->OnConnected(this);
	}

	bool ClientRemoteBase::SendUnreliable(NetworkSegment* pPacket)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
#ifdef LAMBDA_CONFIG_DEBUG
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::SendUnreliable");
#else
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket);
#endif			
			return false;
		}

		GetPacketManager()->EnqueueSegmentUnreliable(pPacket);
		return true;
	}

	bool ClientRemoteBase::SendReliable(NetworkSegment* pPacket, IPacketListener* pListener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
#ifdef LAMBDA_CONFIG_DEBUG
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::SendReliable");
#else
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket);
#endif	
			return false;
		}

		GetPacketManager()->EnqueueSegmentReliable(pPacket, pListener);
		return true;
	}

	bool ClientRemoteBase::SendReliableBroadcast(NetworkSegment* pPacket, IPacketListener* pListener, bool excludeMySelf)
	{
		return m_pServer->SendReliableBroadcast(this, pPacket, pListener, excludeMySelf);
	}

	bool ClientRemoteBase::SendUnreliableBroadcast(NetworkSegment* pPacket, bool excludeMySelf)
	{
		return m_pServer->SendUnreliableBroadcast(this, pPacket, excludeMySelf);
	}

	const ClientMap& ClientRemoteBase::GetClients() const
	{
		return m_pServer->GetClients();
	}

	ServerBase* ClientRemoteBase::GetServer()
	{
		return m_pServer;
	}

	const IPEndPoint& ClientRemoteBase::GetEndPoint() const
	{
		return GetPacketManager()->GetEndPoint();
	}

	NetworkSegment* ClientRemoteBase::GetFreePacket(uint16 packetType)
	{
		SegmentPool* pSegmentPool = GetPacketManager()->GetSegmentPool();

#ifdef LAMBDA_CONFIG_DEBUG
		NetworkSegment* pSegment = pSegmentPool->RequestFreeSegment("ClientRemoteBase");
#else
		NetworkSegment* pSegment = pSegmentPool->RequestFreeSegment();
#endif
		if (pSegment)
		{
			pSegment->SetType(packetType);
		}
		else
		{
			Disconnect("No more free packets!");
		}
		return pSegment;
	}

	void ClientRemoteBase::ReturnPacket(NetworkSegment* pPacket)
	{
#ifdef LAMBDA_CONFIG_DEBUG
		GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::ReturnPacket");
#else
		GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket);
#endif
	}

	EClientState ClientRemoteBase::GetState() const
	{
		return m_State;
	}

	NetworkStatistics* ClientRemoteBase::GetStatistics()
	{
		return GetPacketManager()->GetStatistics();
	}

	IClientRemoteHandler* ClientRemoteBase::GetHandler()
	{
		return m_pHandler;
	}

	uint64 ClientRemoteBase::GetUID() const
	{
		return GetPacketManager()->GetStatistics()->GetSalt();
	}

	void ClientRemoteBase::SetTimeout(Timestamp time)
	{
		m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
		m_PingTimeout = time;
	}

	void ClientRemoteBase::ResetTimeout()
	{
		SetTimeout(m_PingTimeoutDefault);
	}

	void ClientRemoteBase::TransmitPackets()
	{
		GetPacketManager()->Flush(GetTransceiver());
	}

	void ClientRemoteBase::DecodeReceivedPackets()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			TArray<NetworkSegment*> packets;
			PacketManagerBase* pPacketManager = GetPacketManager();
			bool hasDiscardedResends = false;
			if (!pPacketManager->QueryBegin(GetTransceiver(), packets, hasDiscardedResends))
			{
				Disconnect("Receive Error");
				return;
			}

			if (m_State == STATE_CONNECTING)
			{
				if (packets.IsEmpty() && !hasDiscardedResends)
				{
					Disconnect("Expected Connect Packet");
				}
				else
				{
					for (NetworkSegment* pPacket : packets)
					{
						if (pPacket->GetType() != NetworkSegment::TYPE_CONNNECT
							&& pPacket->GetType() != NetworkSegment::TYPE_CHALLENGE
							&& pPacket->GetType() != NetworkSegment::TYPE_DISCONNECT)
						{
							Disconnect("Expected Connect Packet");
							break;
						}
					}
				}
			}

			std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
			for (NetworkSegment* pPacket : packets)
			{
				if (!HandleReceivedPacket(pPacket))
				{
#ifdef LAMBDA_CONFIG_DEBUG
					pPacketManager->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::DecodeReceivedPackets");
#else
					pPacketManager->GetSegmentPool()->FreeSegment(pPacket);
#endif
				}
			}
		}
	}

	void ClientRemoteBase::FixedTick(Timestamp delta)
	{
		GetPacketManager()->Tick(delta);

		if (m_UsePingSystem)
		{
			UpdatePingSystem();
		}

		HandleReceivedPacketsMainThread();
	}

	void ClientRemoteBase::HandleReceivedPacketsMainThread()
	{
		TArray<NetworkSegment*>& packets = m_ReceivedPackets[m_BufferIndex];

		{
			std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
			m_BufferIndex = (m_BufferIndex + 1) % 2;
		}

		for (NetworkSegment* pPacket : packets)
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
		GetPacketManager()->QueryEnd(packets);
		packets.Clear();
	}

	void ClientRemoteBase::UpdatePingSystem()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			Timestamp timeSinceLastPacketReceived = EngineLoop::GetTimeSinceStart() - GetStatistics()->GetTimestampLastReceived();
			if (timeSinceLastPacketReceived >= m_PingTimeout)
			{
				Disconnect("Ping Timed Out");
			}

			if (m_State == STATE_CONNECTED)
			{
				Timestamp timeSinceLastPacketSent = EngineLoop::GetTimeSinceStart() - m_LastPingTimestamp;
				if (timeSinceLastPacketSent >= m_PingInterval)
				{
					m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
					NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_PING);
					if(pSegment)
						SendReliable(pSegment);
				}
			}
		}
	}

	bool ClientRemoteBase::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		//LOG_MESSAGE("ClientRemoteBase::HandleReceivedPacket(%s)", pPacket->ToString().c_str());

		if (packetType == NetworkSegment::TYPE_CONNNECT)
		{
			NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_CHALLENGE);
			if (pSegment)
			{
				GetPacketManager()->EnqueueSegmentUnreliable(pSegment);

				if (!m_pHandler)
				{
					m_pHandler = m_pServer->CreateClientHandler();
					m_pHandler->OnConnecting(this);
				}
			}	
		}
		else if (packetType == NetworkSegment::TYPE_CHALLENGE)
		{
			uint64 expectedAnswer = NetworkChallenge::Compute(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			BinaryDecoder decoder(pPacket);
			uint64 answer;
			if (decoder.ReadUInt64(answer) && answer == expectedAnswer)
			{
				NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_ACCEPTED);
				if (pSegment)
				{
					GetPacketManager()->EnqueueSegmentUnreliable(pSegment);

					if (m_State == STATE_CONNECTING)
					{
						m_State = STATE_CONNECTED;
						m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
					}
				}
			}
			else
			{
				LOG_ERROR("[ClientRemoteBase]: Client responded with %lu, expected %lu, is it a fake client? [%s]", answer, expectedAnswer, GetEndPoint().ToString().c_str());
				Disconnect("Challenge failed!");
			}
		}
		else if (packetType == NetworkSegment::TYPE_DISCONNECT)
		{
			m_DisconnectedByRemote = true;
			Disconnect("Disconnected By Remote");
		}
		else if (packetType == NetworkSegment::TYPE_PING)
		{
			
		}
		else if (IsConnected())
		{
			m_ReceivedPackets[m_BufferIndex].PushBack(pPacket);
			return true;
		}
		return false;
	}

	void ClientRemoteBase::SendDisconnect()
	{
		NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_DISCONNECT);
		if (pSegment)
		{
			GetPacketManager()->EnqueueSegmentUnreliable(pSegment);
			TransmitPackets();
		}
	}

	void ClientRemoteBase::SendServerFull()
	{
		NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_SERVER_FULL);
		if (pSegment)
		{
			GetPacketManager()->EnqueueSegmentUnreliable(pSegment);
			TransmitPackets();
			RequestTermination("Server Is Full");
		}
	}

	void ClientRemoteBase::SendServerNotAccepting()
	{
		NetworkSegment* pSegment = GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING);
		if (pSegment)
		{
			GetPacketManager()->EnqueueSegmentUnreliable(pSegment);
			TransmitPackets();
			RequestTermination("Server Not Accepting");
		}
	}

	void ClientRemoteBase::OnPacketDelivered(NetworkSegment* pPacket)
	{
		LOG_INFO("ClientRemoteBase::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientRemoteBase::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientRemoteBase::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientRemoteBase::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientRemoteBase::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect("Max Tries Reached");
	}

	void ClientRemoteBase::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_ClientRemoteBasesToDelete.empty())
		{
			std::scoped_lock<SpinLock> lock(s_LockStatic);
			TArray<ClientRemoteBase*> networkersToDelete;
			for (auto& pair : s_ClientRemoteBasesToDelete)
			{
				if (--pair.second == 0)
					networkersToDelete.PushBack(pair.first);
			}

			for (ClientRemoteBase* pClient : networkersToDelete)
			{
				s_ClientRemoteBasesToDelete.erase(pClient);
				delete pClient;
			}
		}
	}

	void ClientRemoteBase::ReleaseStatic()
	{
		while (s_Instances > 0)
			FixedTickStatic(EngineLoop::GetFixedTimestep());
	}
}