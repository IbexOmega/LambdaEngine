#include "ECS/EntityPublisher.h"

#include "ECS/ComponentStorage.h"
#include "ECS/System.h"
#include "Log/Log.h"

namespace LambdaEngine
{
    EntityPublisher::EntityPublisher(const ComponentStorage* pComponentStorage, const EntityRegistry* pEntityRegistry)
        :m_pComponentStorage(pComponentStorage),
        m_pEntityRegistry(pEntityRegistry)
    {}

    uint32 EntityPublisher::SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration)
    {
        // Create subscriptions from the subscription requests by finding the desired component containers
        TArray<EntitySubscription> subscriptions;
        const TArray<EntitySubscriptionRegistration>& subscriptionRequests = subscriberRegistration.EntitySubscriptionRegistrations;
        subscriptions.Reserve(subscriptionRequests.GetSize());

        // Convert subscription requests to subscriptions. Also eliminate duplicate TIDs in subscription requests.
        for (const EntitySubscriptionRegistration& subReq : subscriptionRequests)
        {
            const TArray<ComponentAccess>& componentRegs = subReq.ComponentAccesses;

            EntitySubscription newSub;
            newSub.ComponentTypes.Reserve(componentRegs.GetSize());

            newSub.pSubscriber = subReq.pSubscriber;
            newSub.OnEntityAdded = subReq.OnEntityAdded;
            newSub.OnEntityRemoval = subReq.OnEntityRemoval;
            newSub.ExcludedComponentTypes = subReq.ExcludedComponentTypes;

            newSub.ComponentTypes.Reserve(componentRegs.GetSize());
            for (const ComponentAccess& componentReg : componentRegs)
                newSub.ComponentTypes.PushBack(componentReg.pTID);

            EliminateDuplicateTIDs(newSub.ComponentTypes);
            newSub.ComponentTypes.ShrinkToFit();
            subscriptions.EmplaceBack(newSub);
        }

        uint32 subID = m_SystemIDGenerator.GenID();
        m_SubscriptionStorage.PushBack(subscriptions, subID);

        // Map each component type (included and excluded) to its subscriptions
        const TArray<EntitySubscription>& subs = m_SubscriptionStorage.IndexID(subID);

        for (uint32 subscriptionNr = 0; subscriptionNr < subs.GetSize(); subscriptionNr += 1)
        {
            const EntitySubscription& subscription = subs[subscriptionNr];
            const TArray<const ComponentType*>& componentTypes          = subscription.ComponentTypes;
            const TArray<const ComponentType*>& excludedComponentTypes  = subscription.ExcludedComponentTypes;

            for (const ComponentType* pComponentType : componentTypes)
                m_ComponentSubscriptions.insert({ pComponentType, {subID, subscriptionNr}});

            for (const ComponentType* pExcludedComponentType : excludedComponentTypes)
                m_ComponentSubscriptions.insert({ pExcludedComponentType, {subID, subscriptionNr}});
        }

        // A subscription has been made, notify the system of all existing components it subscribed to
        for (EntitySubscription& subscription : subscriptions)
        {
            const ComponentType* pFirstComponentType = subscription.ComponentTypes.GetFront();
            if (!m_pComponentStorage->HasType(pFirstComponentType))
                continue;

            // Fetch the component vector of the first subscribed component type
            const IComponentArray* pComponentArray = m_pComponentStorage->GetComponentArray(pFirstComponentType);
            const TArray<Entity>& entities = pComponentArray->GetIDs();

            // See which entities in the entity vector also have all the other component types. Register those entities in the system.
            for (Entity entity : entities)
            {
                const bool registerEntity = m_pEntityRegistry->EntityHasAllowedTypes(entity, subscription.ComponentTypes, subscription.ExcludedComponentTypes);

                if (registerEntity)
                {
                    subscription.pSubscriber->PushBack(entity);

                    if (subscription.OnEntityAdded != nullptr)
                    {
                        subscription.OnEntityAdded(entity);
                    }
                }
            }
        }

        return subID;
    }

    void EntityPublisher::UnsubscribeFromEntities(uint32 subscriptionID)
    {
        if (m_SubscriptionStorage.HasElement(subscriptionID) == false) {
            LOG_WARNING("Attempted to deregistered an unregistered system, ID: %d", subscriptionID);
            return;
        }

        // Use the subscriptions to find and delete component subscriptions
        const TArray<EntitySubscription>& subscriptions = m_SubscriptionStorage.IndexID(subscriptionID);

        for (const EntitySubscription& subscription : subscriptions)
        {
            const TArray<const ComponentType*>& componentTypes = subscription.ComponentTypes;

            for (const ComponentType* pComponentType : componentTypes)
            {
                auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

                if (subBucketItr == m_ComponentSubscriptions.end())
                {
                    LOG_WARNING("Attempted to delete non-existent component subscription");
                    // The other component subscriptions might exist, so don't return
                    continue;
                }

                // Find the subscription and delete it
                while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
                {
                    if (subBucketItr->second.SystemID == subscriptionID)
                    {
                        m_ComponentSubscriptions.erase(subBucketItr);
                        break;
                    }

                    subBucketItr++;
                }
            }
        }

        // All component->subscription mappings have been deleted
        m_SubscriptionStorage.Pop(subscriptionID);

        // Recycle system iD
        m_SystemIDGenerator.PopID(subscriptionID);
    }

    void EntityPublisher::PublishComponent(Entity entity, const ComponentType* pComponentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_map bucket
        auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
        {
            // Use indices stored in the component type -> component storage mapping to get the component subscription
            EntitySubscription& sysSub = m_SubscriptionStorage.IndexID(subBucketItr->second.SystemID)[subBucketItr->second.SubIdx];

            const bool entityHasExcludedTypes = m_pEntityRegistry->EntityHasAnyOfTypes(entity, sysSub.ExcludedComponentTypes);
            const bool subscriberHasEntity = sysSub.pSubscriber->HasElement(entity);

            // Check if an excluded type was added. If so, remove the entity
            if (subscriberHasEntity && entityHasExcludedTypes)
            {
                if (sysSub.OnEntityRemoval)
                {
                    sysSub.OnEntityRemoval(entity);
                }

                sysSub.pSubscriber->Pop(entity);
            }
            // Check if the entity should be added to the subscription
            else if (!subscriberHasEntity && !entityHasExcludedTypes && m_pEntityRegistry->EntityHasAllTypes(entity, sysSub.ComponentTypes))
            {
                sysSub.pSubscriber->PushBack(entity);

                if (sysSub.OnEntityAdded)
                {
                    sysSub.OnEntityAdded(entity);
                }
            }

            subBucketItr++;
        }
    }

    void EntityPublisher::UnpublishComponent(Entity entity, const ComponentType* pComponentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_multimap bucket
        auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
        {
            // Use indices stored in the component type -> component storage mapping to get the component subscription
            EntitySubscription& sysSub = m_SubscriptionStorage.IndexID(subBucketItr->second.SystemID)[subBucketItr->second.SubIdx];

            if (!sysSub.pSubscriber->HasElement(entity))
            {
                // Check if this component was excluded, and therefore preventing an entity to being pushed to a subscriber
                if (m_pEntityRegistry->EntityHasAllowedTypes(entity, sysSub.ComponentTypes, sysSub.ExcludedComponentTypes))
                {
                    sysSub.pSubscriber->PushBack(entity);

                    if (sysSub.OnEntityAdded)
                    {
                        sysSub.OnEntityAdded(entity);
                    }
                }
            }
            else
            {
                if (sysSub.OnEntityRemoval)
                    sysSub.OnEntityRemoval(entity);

                sysSub.pSubscriber->Pop(entity);
            }

            subBucketItr++;
        }
    }

    void EntityPublisher::EliminateDuplicateTIDs(TArray<const ComponentType*>& TIDs)
    {
        std::unordered_set<const ComponentType*> set;
        for (const ComponentType* pElement : TIDs)
        {
            set.insert(pElement);
        }

        TIDs.Assign(set.begin(), set.end());
    }
}
