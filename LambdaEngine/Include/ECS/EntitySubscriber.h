#pragma once

#include "Containers/TArray.h"
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include <functional>

namespace LambdaEngine
{
    class ECSCore;
    class IDVector;

    enum ComponentPermissions
    {
        NDA = 0,    // No Data Access
        R   = 1,    // Read
        RW  = 2     // Read & Write
    };

    struct ComponentAccess
    {
        ComponentPermissions Permissions;
        std::type_index TID;
    };

    class IComponentGroup
    {
    public:
        virtual TArray<ComponentAccess> ToVector() const = 0;
    };

    // EntitySubscriptionRegistration contains all required information to request a single entity subscription
    class EntitySubscriptionRegistration
    {
    public:
        EntitySubscriptionRegistration(TArray<ComponentAccess> componentAccesses, const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoved = nullptr);
        EntitySubscriptionRegistration(const TArray<ComponentAccess>& componentAccesses, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoved = nullptr);
        EntitySubscriptionRegistration(const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoved = nullptr);

    public:
        TArray<ComponentAccess> ComponentAccesses;
        IDVector* pSubscriber;
        // Optional: Called after an entity was added due to the subscription
        std::function<void(Entity)> OnEntityAdded;
        // Optional: Called before an entity was removed
        std::function<void(Entity)> OnEntityRemoved;
    };

    // EntitySubscriberRegistration is a complete set of data required to register a component subscriber
    struct EntitySubscriberRegistration
    {
        TArray<EntitySubscriptionRegistration> EntitySubscriptionRegistrations;
        /*  AdditionalDependencies are components that the subscriber will process.
            However, the subscriber will not store an array of the entities whose components it will process. */
        TArray<ComponentAccess> AdditionalDependencies;
    };

    // EntitySubscriber deregisters its entity subscriptions at destruction
    class EntitySubscriber
    {
    public:
        EntitySubscriber(ECSCore* pECS);
        ~EntitySubscriber();

        // subscribeToEntities enqueues entity subscriptions. initFn is called when all dependencies have been initialized.
        void SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration, const std::function<bool()>& initFn);

    private:
        ECSCore* m_pECS;
        uint32 m_SubscriptionID;
    };
}