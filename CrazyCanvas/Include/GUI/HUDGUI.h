#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/Image.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/StackPanel.h"
#include "NsGui/ObservableCollection.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "Lobby/Player.h"

#define MAX_AMMO 100

struct GameGUIState
{
	int32 Health;
	int32 MaxHealth = 100;

	LambdaEngine::TArray<uint32> Scores;

	int32 Ammo;
	int32 AmmoCapacity;
};

enum class EPlayerProperty
{
	PLAYER_PROPERTY_NAME,
	PLAYER_PROPERTY_KILLS,
	PLAYER_PROPERTY_DEATHS,
	PLAYER_PROPERTY_FLAGS_CAPTURED,
	PLAYER_PROPERTY_FLAGS_DEFENDED,
	PLAYER_PROPERTY_PING,
};

class HUDGUI : public Noesis::Grid
{
public:
	HUDGUI();
	~HUDGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonShootClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonScoreClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);
	void UpdateCountdown(uint8 countDownTime);

	void DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal);
	void DisplayHitIndicator();
	void DisplayTabMenu(bool visible);

	void AddPlayer(const Player& newPlayer);
	void RemovePlayer(const Player& player);
	void UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value);
	void UpdateAllPlayerProperties(const Player& player);
	void UpdatePlayerAliveStatus(uint64 UID, bool isAlive);

private:
	void InitGUI();

	// Helpers
	void AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column);

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;

	Noesis::Image* m_pWaterAmmoRect = nullptr;
	Noesis::Image* m_pPaintAmmoRect = nullptr;
	
	Noesis::TextBlock* m_pWaterAmmoText = nullptr;
	Noesis::TextBlock* m_pPaintAmmoText = nullptr;

	Noesis::Grid* m_pHitIndicatorGrid	= nullptr;
	Noesis::Grid* m_pTabMenuGrid		= nullptr;

	Noesis::StackPanel* m_pBlueTeamStackPanel	= nullptr;
	Noesis::StackPanel* m_pRedTeamStackPanel	= nullptr;

	bool	m_TabMenuVisible	= false;
};