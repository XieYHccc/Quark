#pragma once
#include "Core/Base.h"
#include "Util/CompileTimeHash.h"
#include "Util/IntrusiveHashMap.h"

/*
A simple Entity-component system immplementation
*/
namespace scene {
using ComponentType = u64;

class Entity;
class Component {
public:
    Component(Entity* entity) : entity_(entity) {};
    virtual ComponentType GetType() = 0;
    virtual ~Component() {};
    Entity* GetEntity() { return entity_; }

protected:
    Entity* entity_;
};

#define QK_COMPONENT_TYPE_DECL(x) \
static inline constexpr scene::ComponentType GetStaticComponentType() { \
    return scene::ComponentType(util::compile_time_fnv1(#x)); \
}\
scene::ComponentType GetType() override {\
    return GetStaticComponentType();\
}

template <typename... Ts>
using ComponentGroup = std::tuple<Ts*...>;

template <typename... Ts>
using ComponentGroupVector = std::vector<ComponentGroup<Ts...>>;

template <typename... Ts>
constexpr u64 GetComponentGroupId(){
    return util::compile_time_fnv1_merged(Ts::GetStaticComponentType()...);
}

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
    T* AddComponent(Ts&&... ts);

    template<typename T>
    void RemoveComponent();
    
private:
    Entity(EntityRegistry* entityRegistry, util::Hash hash);

    EntityRegistry* entityRegistry_;
    size_t EntityRegistryOffset_; // be allocated and used in EntityRegistry
    util::Hash hashId_;
    util::IntrusiveHashMapHolder<util::IntrusivePODWrapper<Component*>> componentMap_;

};

class EntityGroupBase : public util::IntrusiveHashMapEnabled<EntityGroupBase> {
    friend class EntityRegistry;
public:
    EntityGroupBase() = default;
	virtual ~EntityGroupBase() = default;
private:
	virtual void AddEntity(Entity& entity) = 0;
	virtual void RemoveEntity(const Entity& entity) = 0;
	virtual void Reset() = 0;
};

template <typename... Ts>
class EntityGroup final : public EntityGroupBase {
    friend class EntityRegistry;
public:
    const std::vector<Entity*>& GetEntities() const  { return entities_; }
    std::vector<Entity*>& GetEntities() { return entities_; }
    const ComponentGroupVector<Ts...>& GetComponentGroup() const  { return groups_; }
    ComponentGroupVector<Ts...>& GetComponentGroup() { return groups_; }

private:
    void AddEntity(Entity& entity) override final {
		if (has_all_components<Ts...>(entity)) {
			entityToIndex_[entity.GetId()].get() = entities_.size();
			groups_.push_back(std::make_tuple(entity.GetComponent<Ts>()...));
			entities_.push_back(&entity);
		}
    }
    void RemoveEntity(const Entity& entity) override final {
        size_t offset = 0;
        if (entityToIndex_.find_and_consume_pod(entity.GetId(), offset)) {
            entities_[offset] = entities_.back();
            groups_[offset] = groups_.back();
            entityToIndex_[entities_[offset]->GetId()].get() = offset;

            entityToIndex_.erase(entity.GetId());
            entities_.pop_back();
            groups_.pop_back();
        }
    }

    void Reset() override final {
        entities_.clear();
        groups_.clear();
        entityToIndex_.clear();
    }

    ComponentGroupVector<Ts...> groups_;
    std::vector<Entity*> entities_;
    util::IntrusiveHashMap<util::IntrusivePODWrapper<size_t>> entityToIndex_;

	template <typename... Us>
	struct HasAllComponents;

	template <typename U, typename... Us>
	struct HasAllComponents<U, Us...> {
		static bool has_component(const Entity& entity) {
			return entity.HasComponent(U::GetStaticComponentType()) &&
		           HasAllComponents<Us...>::has_component(entity);
		}
	};

	template <typename U>
	struct HasAllComponents<U>{
		static bool has_component(const Entity& entity) {
			return entity.HasComponent(U::GetStaticComponentType());
		}
	};

	template <typename... Us>
	bool has_all_components(const Entity& entity)
	{
		return HasAllComponents<Us...>::has_component(entity);
	}
};

class EntityRegistry {
public:
    ~EntityRegistry();
    EntityRegistry()  = default;
    void operator=(const EntityRegistry &) = delete;
    EntityRegistry(const EntityRegistry &) = delete;

    const std::vector<Entity*>& GetEntities() const { return entities_;}    
    std::vector<Entity*>& GetEntities() { return entities_;}
    
    Entity* CreateEntity();
    void DeleteEntity(Entity* entity);

	template <typename... Ts>
	EntityGroup<Ts...>* GetEntityGroup() {
		ComponentType group_id = GetComponentGroupId<Ts...>();
		auto* t = entityGroups_.find(group_id);
		if (!t) {
			register_group<Ts...>(group_id);

			t = new EntityGroup<Ts...>();
			t->set_hash(group_id);
			entityGroups_.insert_yield(t);

			auto* group = static_cast<EntityGroup<Ts...> *>(t);
			for (auto entity : entities_)
				group->AddEntity(*entity);
		}

		return static_cast<EntityGroup<Ts...> *>(t);
	}

    // Register a component to a entity
    template<typename T, typename... Ts>
    T* Register(Entity* entity, Ts&&... ts )
    {
        auto id = T::GetStaticComponentType();
		auto* t = componentAllocators_.find(id);
		if (!t)
		{
			t = new ComponentAllocator<T>();
			t->set_hash(id);
			componentAllocators_.insert_yield(t);
		}

		auto* allocator = static_cast<ComponentAllocator<T>*>(t);
		auto find = entity->componentMap_.find(id);

		if (find != nullptr) {
			auto* comp = static_cast<T*>(find->get());
			// In-place modify. Destroy old data, and in-place construct.
			// Do not need to fiddle with data structures internally.
			comp->~T();
			return new(comp) T(entity, std::forward<Ts>(ts)...);
		}
		else {
			auto* comp = allocator->pool.allocate(entity, std::forward<Ts>(ts)...);
            auto* node = componentHashedNodePool_.allocate(comp);
            node->set_hash(id);
			entity->componentMap_.insert_replace(node);

			auto* group_set = componentToGroups_.find(id);
			if (group_set)
				for (auto& group : *group_set)
					entityGroups_.find(group.get_hash())->AddEntity(*entity);

			return comp;
		}
    }

    // Unregister a component of a entity
    template<typename T>
    void UnRegister(Entity* entity) { UnRegister(entity, entity->GetComponent<T>()); }
    void UnRegister(Entity* entity, Component* component);

private:
    class ComponentAllocatorBase : public util::IntrusiveHashMapEnabled<ComponentAllocatorBase>
    {
    public:
        virtual ~ComponentAllocatorBase() = default;
        virtual void FreeComponent(Component* component) = 0;
    };

    template <typename T>
    struct ComponentAllocator final : public ComponentAllocatorBase
    {
        util::ObjectPool<T> pool;

        void FreeComponent(Component* component) override final{
            pool.free(static_cast<T*>(component));
        }
    };

    struct GroupKey : util::IntrusiveHashMapEnabled<GroupKey>
    {
 
    };

    class GroupKeySet : public util::IntrusiveHashMapEnabled<GroupKeySet>
    {
    public:
        void insert(ComponentType type);

        util::IntrusiveList<GroupKey>::Iterator begin()
        {
            return set.begin();
        }

        util::IntrusiveList<GroupKey>::Iterator end()
        {
            return set.end();
        }

    private:
        util::IntrusiveHashMap<GroupKey> set;
    };

    util::ObjectPool<Entity> entityPool_;
    util::IntrusiveHashMapHolder<ComponentAllocatorBase> componentAllocators_;
    util::IntrusiveHashMapHolder<EntityGroupBase> entityGroups_;
    util::IntrusiveHashMap<GroupKeySet> componentToGroups_;
    util::ObjectPool<util::IntrusivePODWrapper<Component*>> componentHashedNodePool_;
    std::vector<Entity*> entities_;
    u64 cookie_ = 0;

	template <typename... Us>
	struct GroupRegisters;

	template <typename U, typename... Us>
	struct GroupRegisters<U, Us...>
	{
		static void register_group(util::IntrusiveHashMap<GroupKeySet> &groups,
		                           ComponentType group_id)
		{
			groups.emplace_yield(U::GetStaticComponentType())->insert(group_id);
			GroupRegisters<Us...>::register_group(groups, group_id);
		}
	};

	template <typename U>
	struct GroupRegisters<U>
	{
		static void register_group(util::IntrusiveHashMap<GroupKeySet> &groups,
		                           ComponentType group_id)
		{
			groups.emplace_yield(U::GetStaticComponentType())->insert(group_id);
		}
	};

	template <typename U, typename... Us>
	void register_group(ComponentType group_id)
	{
		GroupRegisters<U, Us...>::register_group(componentToGroups_, group_id);
	}
};

template<typename T, typename... Ts>
T* Entity::AddComponent(Ts&&... ts)
{
    return entityRegistry_->Register<T>(this, ts...);
}

template<typename T>
void Entity::RemoveComponent() 
{
    entityRegistry_->UnRegister<T>(this);
}

}