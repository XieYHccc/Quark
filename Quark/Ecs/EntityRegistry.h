#pragma once
#include "Quark/Ecs/Entity.h"
#include "Quark/Ecs/EntityGroup.h"

namespace quark {
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
            new(comp) T(std::forward<Ts>(ts)...);
            comp->m_Entity = entity;
			return comp;
		}
		else {
			auto* comp = allocator->pool.allocate(std::forward<Ts>(ts)...);
            comp->m_Entity = entity;
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
    return entityRegistry_->Register<T>(this, std::forward<Ts>(ts)...);
}

template<typename T>
void Entity::RemoveComponent() 
{
    entityRegistry_->UnRegister<T>(this);
}
}