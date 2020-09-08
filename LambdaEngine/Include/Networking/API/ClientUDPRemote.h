#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClient.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketManagerUDP.h"

namespace LambdaEngine
{
	class ServerUDP;
	class IClientRemoteHandler;
	class PacketTransceiverUDP;

	class LAMBDA_API ClientUDPRemote : 
		public IClient,
		protected IPacketListener
	{
		friend class ServerUDP;
		
	public:
		~ClientUDPRemote();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkSegment* packet) override;
		virtual bool SendReliable(NetworkSegment* packet, IPacketListener* listener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;

	protected:
		ClientUDPRemote(uint16 packetPoolSize, uint8 maximumTries, const IPEndPoint& ipEndPoint, ServerUDP* pServer);

		virtual PacketManagerBase* GetPacketManager() override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

	private:
		void OnDataReceived(PacketTransceiverUDP* pTransciver);
		void SendPackets(PacketTransceiverUDP* pTransciver);
		bool HandleReceivedPacket(NetworkSegment* pPacket);
		void Tick(Timestamp delta);

	private:
		ServerUDP* m_pServer;
		PacketManagerUDP m_PacketManager;
		SpinLock m_Lock;
		IClientRemoteHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_Release;
		bool m_DisconnectedByRemote;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}