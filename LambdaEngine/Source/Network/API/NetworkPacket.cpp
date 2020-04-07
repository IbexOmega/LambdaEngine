#include "Network/API/NetworkPacket.h"

namespace LambdaEngine
{
	NetworkPacket::NetworkPacket(PACKET_TYPE packetType, bool autoDelete) :
		m_Size(sizeof(m_Size)),
		m_Head(m_Size + sizeof(packetType)),
		m_AutoDelete(autoDelete)
	{
		WriteBuffer((char*)&packetType, sizeof(packetType));
	}

	void NetworkPacket::WriteInt8(int8 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteUInt8(uint8 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt16(int16 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteUInt16(uint16 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt32(int32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteUInt32(uint32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt64(int64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteUInt64(uint64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteFloat32(float32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteFloat64(float64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteBool(bool value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteString(const std::string& value)
	{
		WriteInt16(int16(value.length()));
		WriteBuffer(value.c_str(), uint16(value.length()));
	}

	void NetworkPacket::WriteBuffer(const char* buffer, PACKET_SIZE size)
	{
		memcpy(m_Buffer + m_Size, buffer, size);
		m_Size += size;
	}

	void NetworkPacket::ReadInt8(int8& value)
	{
		ReadBuffer(&value, sizeof(value));
	}

	void NetworkPacket::ReadUInt8(uint8& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadInt16(int16& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadUInt16(uint16& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadInt32(int32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadUInt32(uint32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadInt64(int64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadUInt64(uint64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadFloat32(float32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadFloat64(float64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadBool(bool& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadString(std::string& value)
	{
		int16 length;
		ReadInt16(length);
		value.resize(length);
		ReadBuffer(value.data(), length);
	}

	void NetworkPacket::ReadBuffer(char* buffer, PACKET_SIZE bytesToRead)
	{
		memcpy(buffer, m_Buffer + m_Head, bytesToRead);
		m_Head += bytesToRead;
	}

	int8 NetworkPacket::ReadInt8()
	{
		int8 value = 0;
		ReadInt8(value);
		return value;
	}

	uint8 NetworkPacket::ReadUInt8()
	{
		uint8 value = 0;
		ReadUInt8(value);
		return value;
	}

	int16 NetworkPacket::ReadInt16()
	{
		int16 value = 0;
		ReadInt16(value);
		return value;
	}

	uint16 NetworkPacket::ReadUInt16()
	{
		uint16 value = 0;
		ReadUInt16(value);
		return value;
	}

	int32 NetworkPacket::ReadInt32()
	{
		int32 value = 0;
		ReadInt32(value);
		return value;
	}

	uint32 NetworkPacket::ReadUInt32()
	{
		uint32 value = 0;
		ReadUInt32(value);
		return value;
	}

	int64 NetworkPacket::ReadInt64()
	{
		int64 value = 0;
		ReadInt64(value);
		return value;
	}

	uint64 NetworkPacket::ReadUInt64()
	{
		uint64 value = 0;
		ReadUInt64(value);
		return value;
	}

	float32 NetworkPacket::ReadFloat32()
	{
		float32 value = 0.0f;
		ReadFloat32(value);
		return value;
	}

	float64 NetworkPacket::ReadFloat64()
	{
		float64 value = 0.0f;
		ReadFloat64(value);
		return value;
	}

	bool NetworkPacket::ReadBool()
	{
		bool value = false;
		ReadBool(value);
		return value;
	}

	std::string NetworkPacket::ReadString()
	{
		std::string value;
		ReadString(value);
		return value;
	}

	void NetworkPacket::Reset()
	{
		m_Head = sizeof(PACKET_SIZE) + sizeof(PACKET_TYPE);
		m_Size = m_Head;
	}

	PACKET_SIZE NetworkPacket::GetSize() const
	{
		return m_Size;
	}

	char* NetworkPacket::GetBuffer()
	{
		return m_Buffer;
	}

	void NetworkPacket::Pack()
	{
		memcpy(m_Buffer, (char*)&m_Size, sizeof(m_Size));
	}

	void NetworkPacket::UnPack()
	{
		memcpy(&m_Size, m_Buffer, sizeof(m_Size));
	}

	PACKET_TYPE NetworkPacket::ReadPacketType() const
	{
		PACKET_TYPE value;
		memcpy((char*)&value, m_Buffer + sizeof(PACKET_SIZE), sizeof(value));
		return value;
	}

	bool NetworkPacket::ShouldAutoDelete() const
	{
		return m_AutoDelete;
	}
	const std::string& NetworkPacket::GetAddress() const
	{
		return m_Address;
	}

	uint16 NetworkPacket::GetPort() const
	{
		return m_Port;
	}

	void NetworkPacket::SetDestination(const std::string& address, uint16 port)
	{
		m_Address = address;
		m_Port = port;
	}
}