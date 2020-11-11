#include "States/LobbyState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Chat/ChatManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "Application/API/Events/EventQueue.h"

#include "States/PlaySessionState.h"

using namespace LambdaEngine;

LobbyState::LobbyState(const LambdaEngine::String& name, bool isHost) :
	m_Name(name),
	m_IsHost(isHost)
{

}

LobbyState::~LobbyState()
{
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerHostUpdatedEvent>(this, &LobbyState::OnPlayerHostUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerReadyUpdatedEvent>(this, &LobbyState::OnPlayerReadyUpdatedEvent);
	EventQueue::UnregisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &LobbyState::OnPacketGameSettingsReceived);

	m_LobbyGUI.Reset();
	m_View.Reset();
}

void LobbyState::Init()
{
	EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::RegisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerHostUpdatedEvent>(this, &LobbyState::OnPlayerHostUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerReadyUpdatedEvent>(this, &LobbyState::OnPlayerReadyUpdatedEvent);
	EventQueue::RegisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &LobbyState::OnPacketGameSettingsReceived);

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("PLAYER_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING", true);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	m_LobbyGUI = *new LobbyGUI();
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);

	m_LobbyGUI->InitGUI(m_Name);

	PlayerManagerClient::Reset();
	PlayerManagerClient::RegisterLocalPlayer(m_Name, m_IsHost);
}

void LobbyState::Tick(LambdaEngine::Timestamp delta)
{

}

void LobbyState::FixedTick(LambdaEngine::Timestamp delta)
{

}

bool LobbyState::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerJoinedEvent(%s)", event.pPlayer->GetName().c_str());
	m_LobbyGUI->AddPlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerLeftEvent(%s)", event.pPlayer->GetName().c_str());
	m_LobbyGUI->RemovePlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerStateUpdatedEvent(%s)", event.pPlayer->GetName().c_str());
	const Player* pPlayer = event.pPlayer;
	if (pPlayer->GetState() == GAME_STATE_SETUP)
	{
		if (pPlayer == PlayerManagerClient::GetPlayerLocal())
		{
			State* pStartingState = DBG_NEW PlaySessionState();
			StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
		}
	}
	return false;
}

bool LobbyState::OnPlayerHostUpdatedEvent(const PlayerHostUpdatedEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerHostUpdatedEvent(%s)", event.pPlayer->GetName().c_str());
	m_LobbyGUI->UpdatePlayerHost(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerPingUpdatedEvent(const PlayerPingUpdatedEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerPingUpdatedEvent(%s)", event.pPlayer->GetName().c_str());
	m_LobbyGUI->UpdatePlayerPing(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerReadyUpdatedEvent(const PlayerReadyUpdatedEvent& event)
{
	LOG_ERROR("LobbyState::OnPlayerReadyUpdatedEvent(%s)", event.pPlayer->GetName().c_str());
	m_LobbyGUI->UpdatePlayerReady(*event.pPlayer);
	return false;
}

bool LobbyState::OnChatEvent(const ChatEvent& event)
{
	m_LobbyGUI->WriteChatMessage(event);
	return false;
}

bool LobbyState::OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& packet)
{
	LOG_ERROR("LobbyState::OnPacketGameSettingsReceived()");
	m_LobbyGUI->UpdateSettings(packet.Packet);
	return false;
}
