#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Multiplayer/MultiplayerServer.h"

class Level;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState();
	ServerState(const std::string& serverHostID, const std::string& clientHostID);
	
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	std::string m_ServerName;
	MultiplayerServer m_MultiplayerServer;
};
