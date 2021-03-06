#pragma once
#include "Lobby/PlayerManagerBase.h"

#include "Multiplayer/Packet/PacketPlayerState.h"
#include "Multiplayer/Packet/PacketPlayerReady.h"

#include "Time/API/Timestamp.h"

class PlayerManagerServer : public PlayerManagerBase
{
	friend class CrazyCanvas;
public:
	DECL_STATIC_CLASS(PlayerManagerServer);

	static bool HasPlayerAuthority(const LambdaEngine::IClient* pClient);

	static void SetPlayerState(const Player* pPlayer, EGameState state);
	static void SetPlayerAlive(const Player* pPlayer, bool alive, const Player* pPlayerKiller);
	static void SetPlayerReady(const Player* pPlayer, bool ready);
	static void SetPlayerHost(const Player* pPlayer);
	static void SetPlayerTeam(const Player* pPlayer, uint8 team);
	static void SetPlayerKills(const Player* pPlayer, uint8 kills);
	static void SetPlayerDeaths(const Player* pPlayer, uint8 deaths);
	static void SetPlayerFlagsCaptured(const Player* pPlayer, uint8 flagsCaptured);
	static void SetPlayerFlagsDefended(const Player* pPlayer, uint8 flagsDefended);
	static void SetPlayerStats(const Player* pPlayer, uint8 team, uint8 kills, uint8 deaths, uint8 flagsCaptured, uint8 flagsDefended);
	static void KickPlayers(uint8 count);

	static void Reset();

protected:
	static void Init();
	static void Release();
	static void FixedTick(LambdaEngine::Timestamp deltaTime);

	static bool OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event);
	static bool OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event);
	static bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	static bool OnPacketPlayerStateReceived(const PacketReceivedEvent<PacketPlayerState>& event);
	static bool OnPacketPlayerReadyReceived(const PacketReceivedEvent<PacketPlayerReady>& event);

private:
	static void HandlePlayerLeftServer(LambdaEngine::IClient* pClient);
	static void FillPacketPlayerScore(PacketPlayerScore* pPacket, const Player* pPlayer);
	static void AutoSelectTeam(Player* pPlayer);

protected:
	static LambdaEngine::Timestamp s_Timer;
};