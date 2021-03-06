#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"
#include "ECS/ComponentOwner.h"

#include "Resources/Mesh.h"

#include "Time/API/Clock.h"

#include "Application/API/Events/KeyEvents.h"

namespace LambdaEngine
{
	struct MeshComponent;
	struct AnimationComponent;

	/*
	* AnimationSystem
	*/

	class AnimationSystem : public System, public ComponentOwner
	{
	public:
		bool Init();

		// Inherited from system
		virtual void Tick(Timestamp deltaTime) override final;

		FORCEINLINE float64 GetTotalTimeInSeconds() const
		{
			return m_HasInitClock ? m_Clock.GetTotalTime().AsSeconds() : 0.0;
		}

		FORCEINLINE float64 GetDeltaTimeInSeconds() const
		{
			return m_HasInitClock ? m_Clock.GetDeltaTime().AsSeconds() : 0.0;
		}

	private:
		AnimationSystem();
		~AnimationSystem();

		void Animate(AnimationComponent& animation);
		glm::mat4 ApplyParent(const Joint& joint, Skeleton& skeleton, TArray<glm::mat4>& matrices);

		static void OnAnimationComponentDelete(AnimationComponent& animation, Entity entity);

	public:
		static AnimationSystem& GetInstance();

	private:
		bool m_HasInitClock	= false;
		Clock m_Clock;

		IDVector		m_AnimationEntities;
		IDVector		m_AttachedAnimationEntities;
		TArray<uint32>	m_JobIndices;
	};
}
