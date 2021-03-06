#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

#define MAXIMUM_SEGMENT_SIZE 1024

namespace LambdaEngine
{
	class LAMBDA_API NetworkSegment
	{
		friend class PacketTranscoder;
		friend class SegmentPool;
		friend class PacketManager;
		friend struct NetworkSegmentUIDOrder;

	public:
#pragma pack(push, 1)
		struct Header
		{
			uint16 Size = 0;
			uint16 Type = 0;
			uint32 UID = 0;
			uint32 ReliableUID = 0;
		};
#pragma pack(pop)

		enum Type : uint16
		{
			TYPE_UNDEFINED				= UINT16_MAX - 0, //65 535
			TYPE_PING					= UINT16_MAX - 1,
			TYPE_SERVER_FULL			= UINT16_MAX - 2,
			TYPE_SERVER_NOT_ACCEPTING	= UINT16_MAX - 3,
			TYPE_CONNNECT				= UINT16_MAX - 4,
			TYPE_DISCONNECT				= UINT16_MAX - 5,
			TYPE_CHALLENGE				= UINT16_MAX - 6,
			TYPE_ACCEPTED				= UINT16_MAX - 7,
			TYPE_NETWORK_ACK			= UINT16_MAX - 8,
			TYPE_NETWORK_DISCOVERY		= UINT16_MAX - 9
		};

	public:
		~NetworkSegment();

		NetworkSegment* SetType(uint16 type);
		uint16 GetType() const;

		const uint8* GetBuffer() const;
		uint16 GetBufferSize() const;

		Header& GetHeader();

		uint16 GetTotalSize() const;

		bool Write(const void* pBuffer, uint16 bytes);

		template<typename T>
		bool Write(const T* pStruct)
		{
			return Write(pStruct, sizeof(T));
		}

		bool Read(void* pBuffer, uint16 bytes);

		template<typename T>
		bool Read(T* pStruct)
		{
			return Read(pStruct, sizeof(T));
		}

		uint64 GetRemoteSalt() const;

		bool IsReliable() const;
		uint32 GetReliableUID() const;

		std::string ToString() const;

		void CopyTo(NetworkSegment* pSegment) const;

		void ResetReadHead();

	private:
		NetworkSegment();

	public:
		static constexpr uint8 HeaderSize = sizeof(Header);

	private:
		static void PacketTypeToString(uint16 type, std::string& str);

	private:
#ifdef LAMBDA_CONFIG_DEBUG
		std::string m_Borrower;
		std::string m_Type;
		bool m_IsBorrowed;
#endif

		Header m_Header;
		uint64 m_Salt;
		uint16 m_SizeOfBuffer;
		uint16 m_ReadHead;
		uint8 m_pBuffer[MAXIMUM_SEGMENT_SIZE];
	};

	struct NetworkSegmentReliableUIDOrder
	{
		bool operator()(const NetworkSegment* lhs, const NetworkSegment* rhs) const
		{
			return lhs->GetReliableUID() < rhs->GetReliableUID();
		}
	};

	struct NetworkSegmentUIDOrder
	{
		bool operator()(const NetworkSegment* lhs, const NetworkSegment* rhs) const
		{
			return lhs->m_Header.UID < rhs->m_Header.UID;
		}
	};
}