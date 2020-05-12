#include "Networking/API/PacketPool.h"
#include "Networking/API/NetworkPacket.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketPool::PacketPool(uint16 size)
	{
		m_Packets.reserve(size);
		m_PacketsFree.reserve(size);

		for (uint16 i = 0; i < size; i++)
		{
			NetworkPacket* pPacket = new NetworkPacket();
			m_Packets.push_back(pPacket);
			m_PacketsFree.push_back(pPacket);
		}		
	}

	PacketPool::~PacketPool()
	{
		for (uint16 i = 0; i < m_Packets.size(); i++)
			delete m_Packets[i];

		m_Packets.clear();
	}

	NetworkPacket* PacketPool::RequestFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		NetworkPacket* pPacket = nullptr;
		if (!m_PacketsFree.empty())
		{
			pPacket = m_PacketsFree[m_PacketsFree.size() - 1];
			m_PacketsFree.pop_back();

#ifndef LAMBDA_CONFIG_PRODUCTION
			pPacket->m_IsBorrowed = true;
#endif
		}
		else
		{
			LOG_ERROR("[PacketPool]: No more free packets!, delta = -1");
		}
		return pPacket;
	}

	bool PacketPool::RequestFreePackets(uint16 nrOfPackets, std::vector<NetworkPacket*>& packetsReturned)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		int32 delta = m_PacketsFree.size() - nrOfPackets;

		if (delta < 0)
		{
			LOG_ERROR("[PacketPool]: No more free packets!, delta = %d", delta);
			return false;
		}

		packetsReturned = std::vector<NetworkPacket*>(m_PacketsFree.begin() + delta, m_PacketsFree.end());
		m_PacketsFree = std::vector<NetworkPacket*>(m_PacketsFree.begin(), m_PacketsFree.begin() + delta);

#ifndef LAMBDA_CONFIG_PRODUCTION
		for (int32 i = 0; i < nrOfPackets; i++)
			packetsReturned[i]->m_IsBorrowed = true;
#endif

		return true;
	}

	void PacketPool::FreePacket(NetworkPacket* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

#ifdef LAMBDA_CONFIG_PRODUCTION
		m_PacketsFree.push_back(pPacket);
#else
		if (pPacket->m_IsBorrowed)
		{
			pPacket->m_IsBorrowed = false;
			m_PacketsFree.push_back(pPacket);
		}
		else
		{
			LOG_ERROR("[PacketPool]: Packet was returned multiple times!");
			DEBUGBREAK();
		}
#endif
	}

	void PacketPool::FreePackets(std::vector<NetworkPacket*>& packets)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		for (NetworkPacket* pPacket : packets)
		{

#ifdef LAMBDA_CONFIG_PRODUCTION
			m_PacketsFree.push_back(pPacket);
#else
			if (pPacket->m_IsBorrowed)
			{
				pPacket->m_IsBorrowed = false;
				m_PacketsFree.push_back(pPacket);
			}
			else
			{
				LOG_ERROR("[PacketPool]: Packet was returned multiple times!");
				DEBUGBREAK();
			}
#endif
		}
		packets.clear();
	}

	void PacketPool::Reset()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_PacketsFree.clear();
		m_PacketsFree.reserve(m_Packets.size());

		for (NetworkPacket* pPacket : m_Packets)
		{
#ifndef LAMBDA_CONFIG_PRODUCTION
			pPacket->m_IsBorrowed = false;
#endif
			m_PacketsFree.push_back(pPacket);
		}
	}
}