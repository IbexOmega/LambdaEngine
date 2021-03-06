#pragma once

#include "Game/StateManager.h"

namespace LambdaEngine
{
	class ECSCore;

	class State
	{
	public:
		State();
		State(State* pOther);
		virtual ~State() = 0 {};

		virtual void Init() = 0;

		virtual void Resume() = 0;
		virtual void Pause() = 0;

		virtual void Tick(Timestamp delta) = 0;
		virtual void FixedTick(Timestamp delta);
	};
}
