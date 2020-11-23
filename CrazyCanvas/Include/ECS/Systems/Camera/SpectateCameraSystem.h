#pragma once

#include "ECS/System.h"

#include "Application/API/Events/MouseEvents.h"
#include "Events/PlayerEvents.h"

#include "Containers/TStack.h"


/*
* CameraSystem
*/

class Player;

class SpectateCameraSystem : public LambdaEngine::System
{
public:
	SpectateCameraSystem() = default;
	~SpectateCameraSystem();
	
	void Init();

	void Tick(LambdaEngine::Timestamp deltaTime);
	void FixedTick(LambdaEngine::Timestamp deltaTime);

	bool OnMouseButtonClicked(const LambdaEngine::MouseButtonClickedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

private:
	void SpectatePlayer();

private:
	LambdaEngine::IDVector m_CameraEntities;
	uint8 m_LocalTeamIndex;

	int8 m_SpectatorIndex = 0;

	bool m_InSpectateView = false;

};