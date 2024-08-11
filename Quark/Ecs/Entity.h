#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/CompileTimeHash.h"
#include "Quark/Core/Util/IntrusiveHashMap.h"
#include "Quark/Ecs/Component.h"

// Please include EntityRegistry.h file in you .cpp not this file.
namespace quark {
class EntityRegistry;
class Entity {
public:
    Entity(EntityRegistry* registry, util::Hash hashId)
        : m_Registry(registry), m_HashId(hashId) 
    {

    }

    ~Entity() = default;

    template<typename T>
    bool HasComponent() const { return HasComponent(T::GetStaticComponentType()); }
    bool HasComponent(ComponentType id) const { return m_ComponentMap.find(id) != nullptr; }

    template<typename T>
    T* GetComponent() 
    {
        auto* find = m_ComponentMap.find(T::GetStaticComponentType());
        if (find)
            return static_cast<T*>(find->get());
        else
            return nullptr;;
    }

    template<typename T>
    const T* GetComponent() const 
    {
        auto* find = m_ComponentMap.find(T::GetStaticComponentType());
        if (find)
            return static_cast<T*>(find->get());
        else
            return nullptr;;
    }

    template<typename T, typename... Ts>
    T* AddComponent(Ts&&... ts);    // defined in EntityRegistry.h

    template<typename T>
    void RemoveComponent();
    
private:
    EntityRegistry* m_Registry;
    size_t m_OffsetInRegistry; // be allocated and used in EntityRegistry
    util::Hash m_HashId;    
    util::IntrusiveHashMapHolder<util::IntrusivePODWrapper<Component*>> m_ComponentMap;

    friend class EntityRegistry;
    
    template<typename...>
    friend class EntityGroup;
};

}