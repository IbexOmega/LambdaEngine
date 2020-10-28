#pragma once

#include "Match/MatchBase.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/CreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketGameOver.h"

class MatchClient : public MatchBase
{
public:
	MatchClient() = default;
	~MatchClient();

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) override final;

	bool OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<CreateLevelObject>& event);
	bool OnPacketTeamScoredReceived(const PacketReceivedEvent<PacketTeamScored>& event);
	bool OnPacketGameOverReceived(const PacketReceivedEvent<PacketGameOver>& event);
};