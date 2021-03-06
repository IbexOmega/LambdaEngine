#pragma once
#include "Game/Game.h"

#include "Time/API/Timestamp.h"

#include "Application/API/PlatformApplication.h"

#include <argh/argh.h>

namespace LambdaEngine
{
	/*
	* This class controls the game-loop and makes sure that all engine
	* submodules run at least once per frame
	*/
	class LAMBDA_API EngineLoop
	{
	public:
		DECL_STATIC_CLASS(EngineLoop);

		/*
		* Initializes modules that are needed in EngineLoop::Init()
		*	return - Returns true if successful
		*/
		static bool PreInit(const argh::parser& flagParser);

		/*
		* Initializes all engine modules
		*	return - Returns true if successful
		*/
		static bool Init();

		/*
		* Runs the engine loop, makes sure that ell engine modules get a tick call once per frame
		*/
		static void Run();

		/*
		* Stops/Waits for all execution to stop
		*	return - Returns true if successful
		*/
		static bool PreRelease();

		/*
		* Releases all engine modules
		*	return - Returns true if successful
		*/
		static bool Release();

		/*
		* Releases all modules that are still needed in release
		*	return - Returns true if successful
		*/
		static bool PostRelease();

		static void SetFixedTimestep(Timestamp timestep);
		static Timestamp GetFixedTimestep();

		static Timestamp GetDeltaTime();
		static Timestamp GetTimeSinceStart();

	private:
		/*
		* Initializes ECS systems. Separate from EngineLoop::Init to avoid cluttering.
		*/
		static bool InitSystems();

		/*
		* Engine tick, advances the whole engine one frame. Should only be called from run
		*	delta	- The time between this frame and the last frame
		*	return	- Returns true if the engine should perform next tick
		*/
		static bool Tick(Timestamp delta);

		/*
		* Fixed engine tick, advances the whole engine one frame at a fixed framerate (Current every 16ms).
		* Should only be called from run
		*	delta - The time between this frame and the last frame
		*/
		static void FixedTick(Timestamp delta);
	};
}
