#pragma once
#include "Quark/Ecs/Entity.h"

namespace quark {

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

}