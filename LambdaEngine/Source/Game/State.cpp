#include "Game/State.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    State::State()
    {
    }

    State::State(State* pOther)
    {
        UNREFERENCED_VARIABLE(pOther);
    }

    void State::FixedTick(Timestamp delta)
    {
        UNREFERENCED_VARIABLE(delta);
    }
}
