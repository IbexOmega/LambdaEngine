#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Rendering/EntityMaskManager.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include "Lobby/PlayerManagerClient.h"

#include "Game/StateManager.h"
#include "States/MainMenuState.h"

#include "Teams/TeamHelper.h"

#include "GUI/GUIHelpers.h"

#include "Game/GameConsole.h"
#include "Resources/ResourceCatalog.h"

#include "ECS/Systems/Misc/DestructionSystem.h"

using namespace LambdaEngine;

PlaySessionState* PlaySessionState::s_pInstance = nullptr;

PlaySessionState::PlaySessionState(const PacketGameSettings& gameSettings, bool singlePlayer) :
	m_Singleplayer(singlePlayer),
	m_MultiplayerClient(), 
	m_GameSettings(gameSettings)
{
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Init();
	}

	// Update Team colors and materials
	TeamHelper::SetTeamColor(0, TeamHelper::GetAvailableColor(gameSettings.TeamColor0));
	TeamHelper::SetTeamColor(1, TeamHelper::GetAvailableColor(gameSettings.TeamColor1));

	// Set Team Paint colors
	auto& renderSystem = RenderSystem::GetInstance();
	renderSystem.SetPaintMaskColor(2, TeamHelper::GetTeamColor(0));
	renderSystem.SetPaintMaskColor(1, TeamHelper::GetTeamColor(1));

	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);
}

PlaySessionState::~PlaySessionState()
{
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Release();
	}

	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);

	Match::Release();
}

void PlaySessionState::Init()
{
	s_pInstance = this;

	CommonApplication::Get()->SetMouseVisibility(false);
	PlayerActionSystem::SetMouseEnabled(true);
	Input::PushInputMode(EInputLayer::GAME);

	EnablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Pause();

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Load Match
	{
		const TArray<SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash	= levelHashes[m_GameSettings.MapID],
			.GameMode	= m_GameSettings.GameMode,
			.MaxScore	= m_GameSettings.FlagsToWin,
		};
		Match::CreateMatch(&matchDescription);
	}

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Setup();
	}
	else
	{
		//Called to tell the server we are ready to start the match
		PlayerManagerClient::SetLocalPlayerStateLoading();
		m_CamSystem.Init();
	}

	// Init Systems
	m_HUDSystem.Init();
	m_DestructionSystem.Init();

	// Commands
	ConsoleCommand cmd1;
	cmd1.Init("update_shaders", true);
	cmd1.AddDescription("Update all shaders. \n\t'update_shaders'");
	GameConsole::Get().BindCommand(cmd1, [&, this](GameConsole::CallbackInput& input)->void {
		m_UpdateShaders = true;
	});
}

void PlaySessionState::Tick(Timestamp delta)
{
	if (m_UpdateShaders)
	{
		m_UpdateShaders = false; 
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	m_MultiplayerClient.TickMainThreadInternal(delta);
}

void PlaySessionState::FixedTick(Timestamp delta)
{
	m_HUDSystem.FixedTick(delta);
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
}

bool PlaySessionState::OnClientDisconnected(const ClientDisconnectedEvent& event)
{
	const String& reason = event.Reason;

	LOG_WARNING("PlaySessionState::OnClientDisconnected(Reason: %s)", reason.c_str());

	PlayerManagerClient::Reset();

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}

const PacketGameSettings& PlaySessionState::GetGameSettings() const
{
	return m_GameSettings;
}

PlaySessionState* PlaySessionState::GetInstance()
{
	return s_pInstance;
}