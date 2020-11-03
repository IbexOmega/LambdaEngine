#pragma once
#include "ECS/System.h"

#include "Physics/PhysicsEvents.h"

#include "Threading/API/SpinLock.h"

#define HIT_DAMAGE 20

/*
* HealthSystem
*/

class HealthSystem : public LambdaEngine::System
{
public:
	HealthSystem() = default;
	~HealthSystem() = default;

	virtual void ResetEntityHealth(LambdaEngine::Entity entityToReset)
	{
		UNREFERENCED_VARIABLE(entityToReset);
	}

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) = 0;

protected:
	virtual bool InitInternal();

public:
	static bool Init();
	static void Release();

	FORCEINLINE static HealthSystem& GetInstance() 
	{ 
		VALIDATE(s_pInstance != nullptr);
		return *s_pInstance;
	}

protected:
	LambdaEngine::IDVector m_HealthEntities;

private:
	static HealthSystem* s_pInstance;
};