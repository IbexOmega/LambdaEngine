#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Lobby/PlayerManagerClient.h"

#include "Containers/String.h"

#include "Game/StateManager.h"

#include "States/MultiplayerState.h"

#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "World/LevelManager.h"

using namespace Noesis;
using namespace LambdaEngine;

LobbyGUI::LobbyGUI() : 
	m_IsInitiated(false)
{
	Noesis::GUI::LoadComponent(this, "Lobby.xaml");

	// Get commonly used elements
	m_pBlueTeamStackPanel		= FrameworkElement::FindName<StackPanel>("BlueTeamStackPanel");
	m_pRedTeamStackPanel		= FrameworkElement::FindName<StackPanel>("RedTeamStackPanel");
	m_pChatPanel				= FrameworkElement::FindName<StackPanel>("ChatStackPanel");
	m_pSettingsNamesStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsNamesStackPanel");
	m_pSettingsHostStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsClientStackPanel");
	m_pSettingsClientStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsHostStackPanel");
	m_pChatInputTextBox			= FrameworkElement::FindName<TextBox>("ChatInputTextBox");

	SetHostMode(false);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

void LobbyGUI::InitGUI(LambdaEngine::String name)
{
	AddSettingTextBox(SETTING_SERVER_NAME,      "Server Name");
	AddSettingComboBox(SETTING_MAP,				"Map",					LevelManager::GetLevelNames(), 0);
	AddSettingComboBox(SETTING_MAX_TIME,		"Max Time",				{ "3 min", "5 min", "10 min", "15 min" }, 1);
	AddSettingComboBox(SETTING_FLAGS_TO_WIN,	"Flags To Win",			{ "3", "5", "10", "15" }, 1);
	AddSettingComboBox(SETTING_MAX_PLAYERS,		"Max Players",			{ "4", "6", "8", "10" }, 3);
	AddSettingComboBox(SETTING_VISIBILITY,		"Visibility",			{ "True", "False" }, 1);
	AddSettingComboBox(SETTING_CHANGE_TEAM,		"Allow Change Team",	{ "True", "False" }, 1);

	strcpy(m_GameSettings.ServerName, (name + "'s server").c_str());

	m_IsInitiated = true;
}

void LobbyGUI::AddPlayer(const Player& player)
{
	StackPanel* pPanel = player.GetTeam() == 0 ? m_pBlueTeamStackPanel : m_pRedTeamStackPanel;

	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	// Grid
	Ptr<Grid> playerGrid = *new Grid();
	playerGrid->SetName((uid + "_grid").c_str());
	RegisterName(uid + "_grid", playerGrid);
	ColumnDefinitionCollection* columnCollection = playerGrid->GetColumnDefinitions();
	AddColumnDefinitionStar(columnCollection, 1.f);
	AddColumnDefinitionStar(columnCollection, 7.f);
	AddColumnDefinitionStar(columnCollection, 2.f);

	// Player label
	AddLabelWithStyle(uid + "_name", playerGrid, "PlayerTeamLabelStyle", player.GetName());

	// Ping label
	AddLabelWithStyle(uid + "_ping", playerGrid, "PingLabelStyle", "-");

	// Checkmark image
	Ptr<Image> image = *new Image();
	image->SetName((uid + "_checkmark").c_str());
	RegisterName(uid + "_checkmark", image);
	Style* pStyle = FrameworkElement::FindResource<Style>("CheckmarkImageStyle");
	image->SetStyle(pStyle);
	image->SetVisibility(Visibility::Visibility_Hidden);
	playerGrid->GetChildren()->Add(image);

	pPanel->GetChildren()->Add(playerGrid);
}

void LobbyGUI::RemovePlayer(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Grid* pGrid = m_pBlueTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (pGrid)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(pGrid);

		if (player.IsHost())
			FrameworkElement::GetView()->GetContent()->UnregisterName("host_icon");

		return;
	}

	pGrid = m_pRedTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (pGrid)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(pGrid);

		if(player.IsHost())
			FrameworkElement::GetView()->GetContent()->UnregisterName("host_icon");
	}
}

void LobbyGUI::UpdatePlayerPing(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Label* pPingLabel = FrameworkElement::FindName<Label>((uid + "_ping").c_str());
	if (pPingLabel)
	{
		pPingLabel->SetContent(std::to_string(player.GetPing()).c_str());
	}
}

void LobbyGUI::UpdatePlayerHost(const Player& player)
{
	const Player* pPlayer = PlayerManagerClient::GetPlayerLocal();
	if (&player == pPlayer)
	{
		SetHostMode(player.IsHost());
	}

	// Set host icon
	if (player.IsHost())
	{
		// Host icon is not an image due to AA problems, it is a vector path instead
		Viewbox* crownBox = FrameworkElement::FindName<Viewbox>("host_icon");
		if (crownBox)
		{
			Grid* newGrid = GetPlayerGrid(player);
			Grid* oldGrid = static_cast<Grid*>(crownBox->GetParent());
			oldGrid->GetChildren()->Remove(crownBox);
			newGrid->GetChildren()->Add(crownBox);
		}
		else
		{
			// CreateHostIcon()
			Grid* grid = GetPlayerGrid(player);
			if (grid)
			{
				CreateHostIcon(grid);
			}
			else
			{
				LOG_WARNING("Player %s could not be found when updating player host!", player.GetName().c_str());
			}
		}
	}
}

void LobbyGUI::UpdatePlayerReady(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	// Checkmark styling is currently broken
	Image* pImage = FrameworkElement::FindName<Image>((uid + "_checkmark").c_str());
	if (pImage)
	{
		pImage->SetVisibility(player.IsReady() ? Visibility::Visibility_Visible : Visibility::Visibility_Hidden);
	}
}

void LobbyGUI::WriteChatMessage(const ChatEvent& event)
{
	const ChatMessage& chatMessage = event.Message;

	const LambdaEngine::String& name = event.IsSystemMessage() ? "Server" : chatMessage.Name;

	Ptr<DockPanel> dockPanel = *new DockPanel();

	AddLabelWithStyle("", dockPanel, "ChatNameLabelStyle", name);
	AddLabelWithStyle("", dockPanel, "ChatNameSeperatorStyle", "");

	Ptr<TextBox> message = *new TextBox();
	message->SetText(chatMessage.Message.c_str());
	Style* style = FrameworkElement::FindResource<Style>("ChatMessageStyle");
	message->SetStyle(style);
	dockPanel->GetChildren()->Add(message);

	m_pChatPanel->GetChildren()->Add(dockPanel);
}

void LobbyGUI::SetHostMode(bool isHost)
{
	Button* pReadyButton = FrameworkElement::FindName<Button>("ReadyButton");

	if (isHost)
	{
		pReadyButton->SetContent("Start");
		m_pSettingsClientStackPanel->SetVisibility(Visibility_Hidden);
		m_pSettingsHostStackPanel->SetVisibility(Visibility_Visible);
		ClientHelper::Send(m_GameSettings);
	}
	else
	{
		pReadyButton->SetContent("Ready");
		m_pSettingsClientStackPanel->SetVisibility(Visibility_Visible);
		m_pSettingsHostStackPanel->SetVisibility(Visibility_Hidden);
	}
}

void LobbyGUI::UpdateSettings(const PacketGameSettings& packet)
{
	Label* pSettingServerName = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_SERVER_NAME) + "_client").c_str());
	pSettingServerName->SetContent(packet.ServerName);

	Label* pSettingMap = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAP) + "_client").c_str());
	pSettingMap->SetContent(LevelManager::GetLevelNames()[packet.MapID].c_str());

	Label* pSettingMaxTime = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAX_TIME) + "_client").c_str());
	pSettingMaxTime->SetContent((std::to_string(packet.MaxTime / 60) + " min").c_str());

	Label* pSettingFlagsToWin = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_FLAGS_TO_WIN) + "_client").c_str());
	pSettingFlagsToWin->SetContent(std::to_string(packet.FlagsToWin).c_str());

	Label* pSettingMaxPlayers = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAX_PLAYERS) + "_client").c_str());
	pSettingMaxPlayers->SetContent(std::to_string(packet.Players).c_str());

	Label* pSettingVisibility = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_VISIBILITY) + "_client").c_str());
	pSettingVisibility->SetContent(packet.Visible ? "True" : "False");

	Label* pSettingChangeTeam = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_CHANGE_TEAM) + "_client").c_str());
	pSettingChangeTeam->SetContent(packet.ChangeTeam ? "True" : "False");
}

void LobbyGUI::AddSettingComboBox(
	const LambdaEngine::String& settingKey,
	const LambdaEngine::String& settingText,
	TArray<LambdaEngine::String> settingValues,
	uint8 defaultIndex)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	AddLabelWithStyle(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientStyle", settingValues[defaultIndex]);

	// Add setting combobox
	Ptr<ComboBox> settingComboBox = *new ComboBox();
	Style* pStyle = FrameworkElement::FindResource<Style>("SettingsHostStyle");
	settingComboBox->SetStyle(pStyle);
	settingComboBox->SetName((settingKey + "_host").c_str());
	settingComboBox->SelectionChanged() += MakeDelegate(this, &LobbyGUI::OnComboBoxSelectionChanged);
	RegisterName(settingComboBox->GetName(), settingComboBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingComboBox);

	for (auto& setting : settingValues)
	{
		Ptr<TextBlock> settingTextBlock = *new TextBlock();
		settingTextBlock->SetText(setting.c_str());
		settingComboBox->GetItems()->Add(settingTextBlock);
	}
	settingComboBox->SetSelectedIndex(defaultIndex);
}


void LobbyGUI::AddSettingTextBox(
	const LambdaEngine::String& settingKey,
	const LambdaEngine::String& settingText)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	AddLabelWithStyle(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientStyle", settingText);

	// Add setting textbox
	Ptr<TextBox> settingTextBox = *new TextBox();
	Style* pStyle = FrameworkElement::FindResource<Style>("SettingsHostTextStyle");
	settingTextBox->SetStyle(pStyle);
	settingTextBox->SetName((settingKey + "_host").c_str());
	settingTextBox->TextChanged() += MakeDelegate(this, &LobbyGUI::OnTextBoxChanged);
	RegisterName(settingTextBox->GetName(), settingTextBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingTextBox);
}


bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonReadyClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSendMessageClick);

	return false;
}

void LobbyGUI::OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ToggleButton* pButton = static_cast<ToggleButton*>(pSender);
	PlayerManagerClient::SetLocalPlayerReady(pButton->GetIsChecked().GetValue());
}

void LobbyGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ClientHelper::Disconnect("Leaving lobby");

	State* pMainMenuState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	TrySendChatMessage();
}

bool LobbyGUI::OnKeyPressedEvent(const KeyPressedEvent& event)
{
	if (event.Key == LambdaEngine::EKey::KEY_ENTER)
	{
		if (m_pChatInputTextBox->GetIsFocused())
		{
			TrySendChatMessage();
			return true;
		}
	}
	return false;
}

void LobbyGUI::TrySendChatMessage()
{
	LambdaEngine::String message = m_pChatInputTextBox->GetText();
	if (message.empty())
		return;

	m_pChatInputTextBox->SetText("");
	ChatManager::SendChatMessage(message);
}

void LobbyGUI::OnComboBoxSelectionChanged(Noesis::BaseComponent* pSender, const Noesis::SelectionChangedEventArgs& args)
{
	ComboBox* pComboBox = static_cast<ComboBox*>(pSender);

	LambdaEngine::String setting = pComboBox->GetName();
	setting = setting.substr(0, setting.find_last_of("_"));
	uint32 indexSelected = pComboBox->GetSelectedIndex();
	LambdaEngine::String textSelected = static_cast<TextBlock*>(pComboBox->GetSelectedItem())->GetText();

	if (setting == SETTING_MAP)
	{
		m_GameSettings.MapID = (uint8)indexSelected;
	}
	else if (setting == SETTING_MAX_TIME)
	{
		textSelected = textSelected.substr(0, textSelected.find_last_of(" min"));
		m_GameSettings.MaxTime = (uint16)std::stoi(textSelected) * 60;
	}
	else if (setting == SETTING_FLAGS_TO_WIN)
	{
		m_GameSettings.FlagsToWin = (uint8)std::stoi(textSelected);
	}
	else if (setting == SETTING_MAX_PLAYERS)
	{
		m_GameSettings.Players = (uint8)std::stoi(textSelected);
	}
	else if (setting == SETTING_VISIBILITY)
	{
		m_GameSettings.Visible = textSelected == "True";
	}
	else if (setting == SETTING_CHANGE_TEAM)
	{
		m_GameSettings.ChangeTeam = textSelected == "True";
	}

	if (m_IsInitiated)
	{
		ClientHelper::Send(m_GameSettings);
	}
}

void LobbyGUI::OnTextBoxChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	Noesis::TextBox* pTextBox = static_cast<TextBox*>(pSender);

	if (strcmp(m_GameSettings.ServerName, pTextBox->GetText()) != 0) 
	{
		strcpy(m_GameSettings.ServerName, pTextBox->GetText());

		if (m_IsInitiated)
		{
			ClientHelper::Send(m_GameSettings);
		}
	}
}

void LobbyGUI::AddColumnDefinitionStar(ColumnDefinitionCollection* columnCollection, float width)
{
	GridLength gl = GridLength(width, GridUnitType::GridUnitType_Star);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	columnCollection->Add(col);
}

void LobbyGUI::AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* parent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content)
{
	Ptr<Label> label = *new Label();

	if (name != "")
	{
		label->SetName(name.c_str());
		RegisterName(name, label);
	}

	if (content != "")
		label->SetContent(content.c_str());

	Style* pStyle = FrameworkElement::FindResource<Style>(styleKey.c_str());
	label->SetStyle(pStyle);
	parent->GetChildren()->Add(label);
}

void LobbyGUI::RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* comp)
{
	FrameworkElement::GetView()->GetContent()->RegisterName(name.c_str(), comp);
}

void LobbyGUI::CreateHostIcon(Noesis::Panel* parent)
{
	Ptr<Viewbox> viewBox	= *new Viewbox();
	Ptr<Path> path			= *new Path();

	Style* pStyle = FrameworkElement::FindResource<Style>("CrownPath");
	path->SetStyle(pStyle);
	viewBox->SetChild(path);

	// viewBox->SetNodeParent(parent);
	parent->GetChildren()->Add(viewBox);

	RegisterName("host_icon", viewBox);
}

Noesis::Grid* LobbyGUI::GetPlayerGrid(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Grid* pGrid = m_pBlueTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (pGrid)
	{
		return pGrid;
	}

	pGrid = m_pRedTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	return pGrid;
}
