#include "Networking/API/IPEndPoint.h"

#include "Networking/API/IPAddress.h" 

namespace LambdaEngine
{
	IPEndPoint::IPEndPoint() : IPEndPoint(IPAddress::NONE, 0)
	{

	}

	IPEndPoint::IPEndPoint(IPAddress* pAddress, uint16 port) :
		m_pAddress(pAddress),
		m_Port(port),
		m_Hash(0)
	{
		UpdateHash();
	}

	IPEndPoint::~IPEndPoint()
	{
		
	}

	void IPEndPoint::SetEndPoint(IPAddress* pAddress, uint16 port)
	{
		if (pAddress != m_pAddress)
		{
			m_pAddress = pAddress;
			if (port != m_Port)
			{
				m_Port = port;
			}
			UpdateHash();
		}
		else if (port != m_Port)
		{
			m_Port = port;
			UpdateHash();
		}
	}

	void IPEndPoint::SetAddress(IPAddress* pAddress)
	{
		if (pAddress != m_pAddress)
		{
			m_pAddress = pAddress;
			UpdateHash();
		}
	}

	IPAddress* IPEndPoint::GetAddress() const
	{
		return m_pAddress;
	}

	void IPEndPoint::SetPort(uint16 port)
	{
		if (port != m_Port)
		{
			m_Port = port;
			UpdateHash();
		}
	}

	uint16 IPEndPoint::GetPort() const
	{
		return m_Port;
	}

	std::string IPEndPoint::ToString() const
	{
		return m_pAddress->ToString() + ":" + std::to_string(m_Port);
	}

	uint64 IPEndPoint::GetHash() const
	{
		return m_Hash;
	}

	bool IPEndPoint::IsValid() const
	{
		return m_pAddress->IsValid();
	}

	bool IPEndPoint::operator==(const IPEndPoint& other) const
	{
		return m_Hash == other.m_Hash/*other.GetAddress()->GetHash() == GetAddress()->GetHash() && other.GetPort() == GetPort()*/;
	}

	bool IPEndPoint::operator!=(const IPEndPoint& other) const
	{
		return m_Hash != other.m_Hash;
	}

	bool IPEndPoint::operator<(const IPEndPoint& other) const
	{
		return GetHash() < other.GetHash();
	}

	void IPEndPoint::UpdateHash()
	{
		uint64 port64 = m_Port;
		m_Hash = port64 ^ m_pAddress->GetHash() + 0x9e3779b9 + (port64 << 6) + (port64 >> 2);
	}

	IPEndPoint IPEndPoint::Parse(const String& string, uint16 defaultPort, bool* result)
	{
		String addressStr;
		String portStr;
		bool addressFound = false;

		for (const char c : string)
		{
			if (c == ':')
				addressFound = true;
			else if (addressFound)
				portStr += c;
			else
				addressStr += c;
		}

		IPAddress* pAddress = IPAddress::Get(addressStr);
		uint16 port = portStr.empty() ? defaultPort : (uint16)std::stoi(portStr);

		*result = pAddress->IsValid();
		
		return IPEndPoint(pAddress, port);
	}
}