#include "Networking/API/NetworkStatistics.h"

#include "Engine/EngineLoop.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	NetworkStatistics::NetworkStatistics()
	{
		Reset();
	}

	NetworkStatistics::~NetworkStatistics()
	{

	}

	uint32 NetworkStatistics::GetPacketsSent() const
	{
		return m_PacketsSent;
	}

	uint32 NetworkStatistics::GetSegmentsRegistered() const
	{
		return m_SegmentsRegistered;
	}

	uint32 NetworkStatistics::GetSegmentsSent() const
	{
		return m_SegmentsSent;
	}

	uint32 NetworkStatistics::GetReliableSegmentsSent() const
	{
		return m_ReliableSegmentsSent;
	}

	uint32 NetworkStatistics::GetPacketsReceived() const
	{
		return m_PacketsReceived;
	}

	uint32 NetworkStatistics::GetSegmentsReceived() const
	{
		return m_SegmentsReceived;
	}

	uint32 NetworkStatistics::GetPacketsLost() const
	{
		return m_PacketsLost;
	}

	float64 NetworkStatistics::GetPacketLossRate() const
	{
		if (m_PacketsSent == 0)
			return 0.0F;
		return m_PacketsLost / (float64)m_PacketsSent;
	}

	uint32 NetworkStatistics::GetBytesSent() const
	{
		return m_BytesSent;
	}

	uint32 NetworkStatistics::GetBytesReceived() const
	{
		return m_BytesReceived;
	}

	const Timestamp& NetworkStatistics::GetPing() const
	{
		return m_Ping;
	}

	uint64 NetworkStatistics::GetSalt() const
	{
		return m_Salt;
	}

	uint64 NetworkStatistics::GetRemoteSalt() const
	{
		return m_SaltRemote;
	}

	uint32 NetworkStatistics::GetLastReceivedSequenceNr() const
	{
		return m_LastReceivedSequenceNr;
	}

	uint64 NetworkStatistics::GetReceivedSequenceBits() const
	{
		return m_ReceivedSequenceBits;
	}

	uint32 NetworkStatistics::GetLastReceivedAckNr() const
	{
		return m_LastReceivedAckNr;
	}

	uint64 NetworkStatistics::GetReceivedAckBits() const
	{
		return m_ReceivedAckBits;
	}

	uint32 NetworkStatistics::GetLastReceivedReliableUID() const
	{
		return m_LastReceivedReliableUID;
	}

	Timestamp NetworkStatistics::GetTimestampLastSent() const
	{
		return m_TimestampLastSent;
	}

	Timestamp NetworkStatistics::GetTimestampLastReceived() const
	{
		return m_TimestampLastReceived;
	}

	void NetworkStatistics::Reset()
	{
		m_Salt						= Random::UInt64();
		m_SaltRemote				= 0;
		m_Ping						= Timestamp::MilliSeconds(10.0f);
		m_PacketsSent				= 0;
		m_SegmentsRegistered		= 0;
		m_SegmentsSent				= 0;
		m_ReliableSegmentsSent		= 0;
		m_PacketsReceived			= 0;
		m_SegmentsReceived			= 0;
		m_PacketsLost				= 0;
		m_BytesSent					= 0;
		m_BytesReceived				= 0;
		m_LastReceivedSequenceNr	= 0;
		m_ReceivedSequenceBits		= 0;
		m_LastReceivedAckNr			= 0;
		m_ReceivedAckBits			= 0;
		m_LastReceivedReliableUID	= 0;
		m_TimestampLastSent			= 0;
		m_TimestampLastReceived		= 0;
	}

	uint32 NetworkStatistics::RegisterPacketSent()
	{
		m_TimestampLastSent = EngineLoop::GetTimeSinceStart();
		return ++m_PacketsSent;
	}

	uint32 NetworkStatistics::RegisterUniqueSegment()
	{
		return ++m_SegmentsRegistered;
	}

	void NetworkStatistics::RegisterSegmentSent(uint32 segments)
	{
		m_SegmentsSent += segments;
	}

	uint32 NetworkStatistics::RegisterReliableSegmentSent()
	{
		return ++m_ReliableSegmentsSent;
	}

	void NetworkStatistics::RegisterPacketReceived(uint32 segments, uint32 bytes)
	{
		m_PacketsReceived++;
		m_SegmentsReceived += segments,
		m_BytesReceived += bytes;
		m_TimestampLastReceived = EngineLoop::GetTimeSinceStart();
	}

	void NetworkStatistics::RegisterReliableSegmentReceived()
	{
		m_LastReceivedReliableUID++;
	}

	void NetworkStatistics::RegisterPacketLoss()
	{
		m_PacketsLost++;
	}

	void NetworkStatistics::RegisterBytesSent(uint32 bytes)
	{
		m_BytesSent += bytes;
	}

	void NetworkStatistics::SetLastReceivedSequenceNr(uint32 sequence)
	{
		m_LastReceivedSequenceNr = sequence;
	}

	void NetworkStatistics::SetReceivedSequenceBits(uint64 sequenceBits)
	{
		m_ReceivedSequenceBits = sequenceBits;
	}

	void NetworkStatistics::SetLastReceivedAckNr(uint32 ack)
	{
		m_LastReceivedAckNr = ack;
	}

	void NetworkStatistics::SetReceivedAckBits(uint64 ackBits)
	{
		m_ReceivedAckBits = ackBits;
	}

	void NetworkStatistics::SetRemoteSalt(uint64 salt)
	{
		m_SaltRemote = salt;
	}
}