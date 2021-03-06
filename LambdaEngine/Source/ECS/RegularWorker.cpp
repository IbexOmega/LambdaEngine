#include "ECS/RegularWorker.h"

#include "Containers/TArray.h"
#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	RegularWorker::~RegularWorker()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		if (pECS && m_JobID != UINT32_MAX)
			pECS->DescheduleRegularJob(m_Phase, m_JobID);
	}

	void RegularWorker::Tick()
	{
		const Timestamp deltaTime = m_TickPeriod > 0.0f ? Timestamp::Seconds(m_TickPeriod) : ECSCore::GetInstance()->GetDeltaTime();
		m_TickFunction(deltaTime);
	}

	void RegularWorker::ScheduleRegularWork(const RegularWorkInfo& regularWorkInfo)
	{
		m_Phase = regularWorkInfo.Phase;
		m_TickPeriod = regularWorkInfo.TickPeriod;
		m_TickFunction = regularWorkInfo.TickFunction;

		const RegularJob regularJob =
		{
			/* Components */	RegularWorker::GetUniqueComponentAccesses(regularWorkInfo.EntitySubscriberRegistration),
			/* Function */		std::bind(&RegularWorker::Tick, this),
			/* TickPeriod */	m_TickPeriod,
			/* Accumulator */	0.0f
		};

		m_JobID = ECSCore::GetInstance()->ScheduleRegularJob(regularJob, m_Phase);
	}

	TArray<ComponentAccess> RegularWorker::GetUniqueComponentAccesses(const EntitySubscriberRegistration& subscriberRegistration)
	{
		// Eliminate duplicate component types across the system's subscriptions
		THashTable<const ComponentType*, ComponentPermissions> uniqueRegs;

		for (const EntitySubscriptionRegistration& subReq : subscriberRegistration.EntitySubscriptionRegistrations)
		{
			RegularWorker::MapComponentAccesses(subReq.ComponentAccesses, uniqueRegs);
		}

		RegularWorker::MapComponentAccesses(subscriberRegistration.AdditionalAccesses, uniqueRegs);

		// Merge all of the system's subscribed component types into one vector
		TArray<ComponentAccess> componentAccesses;
		componentAccesses.Reserve((uint32)uniqueRegs.size());
		for (auto& uniqueRegsItr : uniqueRegs)
		{
			componentAccesses.PushBack({uniqueRegsItr.second, uniqueRegsItr.first});
		}

		return componentAccesses;
	}

	void RegularWorker::MapComponentAccesses(const TArray<ComponentAccess>& componentAccesses, THashTable<const ComponentType*, ComponentPermissions>& uniqueRegs)
	{
		for (const ComponentAccess& componentUpdateReg : componentAccesses)
		{
			if (componentUpdateReg.Permissions == NDA)
			{
				continue;
			}

			auto uniqueRegsItr = uniqueRegs.find(componentUpdateReg.pTID);
			if (uniqueRegsItr == uniqueRegs.end() || componentUpdateReg.Permissions > uniqueRegsItr->second)
			{
				uniqueRegs.insert(uniqueRegsItr, {componentUpdateReg.pTID, componentUpdateReg.Permissions});
			}
		}
	}
}
