#pragma once
#include "Quark/Ecs/Entity.h"

namespace quark {

class EntityGroupBase : public util::IntrusiveHashMapEnabled<EntityGroupBase> {
public:
    EntityGroupBase() = default;
	virtual ~EntityGroupBase() = default;

	virtual void EntityAdd(Entity& entity) = 0;
	virtual void EntityRemove(const Entity& entity) = 0;
	virtual void Reset() = 0;
};

template <typename... Ts>
class EntityGroup final : public EntityGroupBase {
public:
    const std::vector<Entity*>& GetEntities() const  { return m_Entities; }
    std::vector<Entity*>& GetEntities() { return m_Entities; }
    const ComponentGroupVector<Ts...>& GetComponentGroup() const  { return m_ComponentGroups; }
    ComponentGroupVector<Ts...>& GetComponentGroup() { return m_ComponentGroups; }

    void EntityAdd(Entity& entity) override final {
		if (has_all_components<Ts...>(entity)) {
			m_EntityToIndexMap[entity.m_HashId].get() = m_Entities.size();
			m_ComponentGroups.push_back(std::make_tuple(entity.GetComponent<Ts>()...));
			m_Entities.push_back(&entity);
		}
    }
    void EntityRemove(const Entity& entity) override final {
        size_t offset = 0;
        if (m_EntityToIndexMap.find_and_consume_pod(entity.m_HashId, offset)) {
            m_Entities[offset] = m_Entities.back();
            m_ComponentGroups[offset] = m_ComponentGroups.back();
            m_EntityToIndexMap[m_Entities[offset]->m_HashId].get() = offset;

            m_EntityToIndexMap.erase(entity.m_HashId);
            m_Entities.pop_back();
            m_ComponentGroups.pop_back();
        }
    }

    void Reset() override final {
        m_Entities.clear();
        m_ComponentGroups.clear();
        m_EntityToIndexMap.clear();
    }

private:
    ComponentGroupVector<Ts...> m_ComponentGroups;
    std::vector<Entity*> m_Entities;
    util::IntrusiveHashMap<util::IntrusivePODWrapper<size_t>> m_EntityToIndexMap;

	template <typename... Us>
	struct HasAllComponents;

	template <typename U, typename... Us>
	struct HasAllComponents<U, Us...> 
	{
		static bool has_component(const Entity& entity) 
		{
			return entity.HasComponent(U::GetStaticComponentType()) &&
		           HasAllComponents<Us...>::has_component(entity);
		}
	};

	template <typename U>
	struct HasAllComponents<U>
	{
		static bool has_component(const Entity& entity) 
		{
			return entity.HasComponent(U::GetStaticComponentType());
		}
	};

	template <typename... Us>
	bool has_all_components(const Entity& entity)
	{
		return HasAllComponents<Us...>::has_component(entity);
	}
};

}