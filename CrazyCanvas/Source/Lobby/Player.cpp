#include "Lobby/Player.h"

using namespace LambdaEngine;

Player::Player()
	: m_Name()
	, m_Entity(UINT32_MAX)
	, m_IsHost(false)
	, m_IsDead(false)
	, m_IsReady(false)
	, m_IsSpectator(false)
	, m_Ping()
	, m_State(GAME_STATE_LOBBY)
	, m_Team(0)
	, m_Kills(0)
	, m_Deaths(0)
	, m_FlagsCaptured(0)
	, m_FlagsDefended(0)
	, m_UID(0)
{
}

const String& Player::GetName() const
{
	return m_Name;
}

LambdaEngine::Entity Player::GetEntity() const
{
	return m_Entity;
}

bool Player::IsHost() const
{
	return m_IsHost;
}

bool Player::IsDead() const
{
	return m_IsDead;
}

bool Player::IsReady() const
{
	return m_IsReady;
}

bool Player::IsSpectator() const
{
	return m_IsSpectator;
}

uint16 Player::GetPing() const
{
	return m_Ping;
}

EGameState Player::GetState() const
{
	return m_State;
}

uint8 Player::GetTeam() const
{
	return m_Team;
}

uint8 Player::GetKills() const
{
	return m_Kills;
}

uint8 Player::GetDeaths() const
{
	return m_Deaths;
}

uint8 Player::GetFlagsCaptured() const
{
	return m_FlagsCaptured;
}

uint8 Player::GetFlagsDefended() const
{
	return m_FlagsDefended;
}

uint64 Player::GetUID() const
{
	return m_UID;
}

bool Player::operator==(const Player& other) const
{
	return m_UID == other.m_UID;
}

bool Player::operator!=(const Player& other) const
{
	return m_UID != other.m_UID;
}

bool Player::operator<(const Player& other) const
{
	return m_UID < other.m_UID;
}