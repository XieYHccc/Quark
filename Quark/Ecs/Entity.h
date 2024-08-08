#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/CompileTimeHash.h"
#include "Quark/Core/Util/IntrusiveHashMap.h"
#include "Quark/Ecs/Component.h"

// Please include EntityRegistry.h file in you .cpp not this file
namespace quark {

class EntityRegistry;
class Entity {
    friend class EntityRegistry;
    friend class util::ObjectPool<Entity>;
public:
    Entity() = delete;
    ~Entity() = default;

    util::Hash GetId() const { return hashId_;};

    bool HasComponent(ComponentType id) const {
        auto find = componentMap_.find(id);
        return find != nullptr;
    }

    template<typename T>
    bool HasComponent() const { return HasComponent(T::GetStaticComponentType()); }

    template<typename T>
    T* GetComponent() {
        auto* find = componentMap_.find(T::GetStaticComponentType());
        if (find)
            return static_cast<T*>(find->get());
        else
            return nullptr;;
    }

    template<typename T>
    const T* GetComponent() const {
        auto* find = componentMap_.find(T::GetStaticComponentType());
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
    Entity(EntityRegistry* entityRegistry, util::Hash hash);

    EntityRegistry* entityRegistry_;
    size_t entityRegistryOffset_; // be allocated and used in EntityRegistry
    util::Hash hashId_;
    util::IntrusiveHashMapHolder<util::IntrusivePODWrapper<Component*>> componentMap_;

};

}