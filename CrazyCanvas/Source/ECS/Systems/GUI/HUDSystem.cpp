#include "ECS/Systems/GUI/HUDSystem.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"


#include "Application/API/Events/EventQueue.h"


using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();
}

void HUDSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_WeaponEntities,
			.ComponentAccesses =
			{
				{ R, WeaponComponent::Type() },
			}
		},
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{ R, HealthComponent::Type() }, { NDA, PlayerLocalComponent::Type() }
			}
		}
	};
	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);


	m_HUDGUI = *new HUDGUI("HUD.xaml");
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	GUIApplication::SetView(m_View);
}

void HUDSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
}

void HUDSystem::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	const ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();


	for (Entity players : m_PlayerEntities)
	{
		const HealthComponent& healthComponent = pHealthComponents->GetConstData(players);
		m_HUDGUI->UpdateScore();
		m_HUDGUI->UpdateHealth(healthComponent.CurrentHealth);
	}
}

bool HUDSystem::OnWeaponFired(const WeaponFiredEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

	for (Entity playerWeapon : m_WeaponEntities)
	{
		const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

		if (pPlayerLocalComponents->HasComponent(event.WeaponOwnerEntity) && m_HUDGUI)
		{
			m_HUDGUI->UpdateAmmo(weaponComponent.CurrentAmmunition, weaponComponent.AmmoCapacity, event.AmmoType);
		}
	}

	return false;
}
