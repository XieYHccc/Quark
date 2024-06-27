#pragma once
#include "Util/IntrusiveHashMap.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Component.h"

namespace scene {

class EntityRegistry;
class Entity : public util::IntrusiveListEnabled<Entity>{
    friend class EntityRegistry;
    friend class Scene;
    friend class util::ObjectPool<Entity>;
public:
    Entity() = delete;
    ~Entity() = default;

    util::Hash GetId() const { return hashId_;};

    void SetParent(Entity* parent) { parent_ = parent;}
    Entity* GetParent() { return parent_; }

    void AddChild(Entity* child) { children_.insert_front(child);}
    void RemoveChild(Entity* child) { children_.erase(child); }

    template<typename T>
    bool HasComponent() {
        auto find = componentMap_.find(T::GetStaticComponentType());
        return find != componentMap_.end();
    }
    
    template<typename T>
    T* GetComponent() {
        auto find = componentMap_.find(T::GetStaticComponentType());
        if (find != componentMap_.end()) 
            return static_cast<T*>(find->second);
        else
            return nullptr;
    }

    std::unordered_map<ComponentType, Component*>& GetComponentsMap() { return componentMap_; }

    template<typename T, typename... Ts>
    T* AddComponent(Ts&&... ts);

    template<typename T>
    void RemoveComponent();
    
private:
    Entity(EntityRegistry* registry, util::Hash hash, const std::string& name);

    EntityRegistry* registry_;
    size_t registryOffset_; // be allocated and used in Registry
    std::string name_;
    util::Hash hashId_;
    Entity* parent_;
    util::IntrusiveList<Entity> children_;
    TransformCmpt* transformCmpt_;
    std::unordered_map<ComponentType, Component*> componentMap_;

};

template <typename... Ts>
using ComponentGroup = std::tuple<Ts*...>;

template <typename... Ts>
using ComponentGroupVector = std::vector<ComponentGroup<Ts...>>;

template <typename... Ts>
constexpr u64 GetComponentGroupId(){
    return util::compile_time_fnv1_merged(Ts::GetComponentTypeHash()...);
}

class EntityGroupBase : public util::IntrusiveHashMapEnabled<EntityGroupBase> {
    friend class EntityRegistry;
public:
    EntityGroupBase() = default;
	virtual ~EntityGroupBase() = default;
private:
	virtual void AddEntity(const Entity* entity) = 0;
	virtual void RemoveEntity(const Entity* entity) = 0;
	virtual void Reset() = 0;
};

template <typename... Ts>
class EntityGroup final : public EntityGroupBase {
public:
    const std::vector<Entity*>& GetEntities() { return entities_;}
    const ComponentGroupVector<Ts...>& GetComponentGroup() { return groups_;}

private:
    void AddEntity(const Entity* entity) override final {
		if (has_all_components<Ts...>(entity)) {
			entityToIndex_[entity->GetId()].get() = entities_.size();
			groups_.push_back(std::make_tuple(entity->GetComponent<Ts>()...));
			entities_.push_back(entity);
		}
    }
    void RemoveEntity(const Entity* entity) override final {
        size_t offset = 0;
        if (entityToIndex_.find_and_consume_pod(entity->GetId(), offset)) {
            entities_[offset] = entities_.back();
            groups_[offset] = groups_.back;
            entityToIndex_[entities_[offset]->GetId()].get() = offset;

            entityToIndex_.erase(entity->GetId());
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
		static bool has_component(const Entity &entity) {
			return entity.HasComponent<U::GetComponentType>() &&
		           HasAllComponents<Us...>::has_component(entity);
		}
	};

	template <typename U>
	struct HasAllComponents<U>{
		static bool has_component(const Entity &entity) {
			return entity.HasComponent<U::GetComponentType>();
		}
	};

	template <typename... Us>
	bool has_all_components(const Entity &entity)
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

    Entity* CreateEntity(const std::string& name = "");
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
			for (auto& entity : entities_)
				group->AddEntity(*entity);
		}

		return static_cast<EntityGroup<Ts...> *>(t);
	}

    // Register a component to a entity
    template<typename T, typename... Ts>
    T* Register(Entity* entity, Ts&&... ts )
    {
        auto component_type = T::GetStaticComponentType();
		auto* t = componentAllocators_.find(component_type);
		if (!t)
		{
			t = new ComponentAllocator<T>();
			t->set_hash(component_type);
			componentAllocators_.insert_yield(t);
		}

		auto* allocator = static_cast<ComponentAllocator<T>*>(t);
		auto find = entity->componentMap_.find(component_type);

		if (find != entity->componentMap_.end()) {
			auto* comp = static_cast<T*>(find->second);
			// In-place modify. Destroy old data, and in-place construct.
			// Do not need to fiddle with data structures internally.
			comp->~T();
			return new(comp) T(std::forward<Ts>(ts)...);
		}
		else {
			auto* comp = allocator->pool.allocate(std::forward<Ts>(ts)...);
			entity->componentMap_.emplace(std::make_pair(component_type, comp));

			auto* group_set = componentToGroups_.find(component_type);
			if (group_set)
				for (auto& group : *group_set)
					entityGroups_.find(group.get_hash())->AddEntity(entity);

			return comp;
		}
    }

    // Unregister a component of a entity
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
    return registry_->Register<T>(this, ts...);
}

template<typename T>
void Entity::RemoveComponent() 
{
    registry_->UnRegister(this, GetComponent<T>());
}

}