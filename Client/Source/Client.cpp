#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Client::Client() : 
    m_pClient(nullptr)
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

    m_pClient = ClientUDP::Create(this, 512);

    if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
    {
        LOG_ERROR("Failed to connect!");
    }
}

Client::~Client()
{
    m_pClient->Release();
}

void Client::OnConnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_PARAMETER(pClient);
    LOG_MESSAGE("OnConnectingUDP()");
}

void Client::OnConnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_PARAMETER(pClient);
    LOG_MESSAGE("OnConnectedUDP()");
}

void Client::OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_PARAMETER(pClient);
    LOG_MESSAGE("OnDisconnectingUDP()");
}

void Client::OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_PARAMETER(pClient);
    LOG_MESSAGE("OnDisconnectedUDP()");
}

void Client::OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_PARAMETER(pClient);
    UNREFERENCED_PARAMETER(pPacket);
    LOG_MESSAGE("OnPacketReceivedUDP()");
}

void Client::OnPacketDelivered(LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_PARAMETER(pPacket);
    LOG_INFO("OnPacketDelivered()");
}

void Client::OnPacketResent(LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_PARAMETER(pPacket);
    LOG_INFO("OnPacketResent()");
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(key);

    if (key == EKey::KEY_ENTER)
    {
        if (m_pClient->IsConnected())
            m_pClient->Disconnect();
        else
            m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
    }
    else
    {
        uint16 packetType = 0;
        NetworkPacket* packet = m_pClient->GetFreePacket(packetType);
        BinaryEncoder encoder(packet);
        encoder.WriteString("Test Message");
        m_pClient->SendReliable(packet, this);
    }
}

void Client::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Client::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Client::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Client::FixedTick(LambdaEngine::Timestamp delta)
{
    using namespace LambdaEngine;
    UNREFERENCED_VARIABLE(delta);

    m_pClient->Flush();
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Client* pSandbox = DBG_NEW Client();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
