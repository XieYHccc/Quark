#include "pch.h"
#include "Scene/Ecs.h"

namespace scene {

Entity::Entity(EntityRegistry* EntityRegistry, util::Hash hash)
    : EntityRegistry_(EntityRegistry), hashId_(hash)
{
}

void EntityRegistry::UnRegister(Entity* entity, Component* component)
{
    if (component == nullptr)
        return;
    CORE_DEBUG_ASSERT(component->GetEntity() == entity) // Double check

    entity->componentMap_.erase(component->GetType());

    auto* group_key_set = componentToGroups_.find(component->GetType());
    if (group_key_set) {
        for (auto &group_key : *group_key_set) {
            auto* group = entityGroups_.find(group_key.get_hash());
            if (group)
                group->RemoveEntity(entity);
        }
    }  

    auto* allocator = componentAllocators_.find(component->GetType());
    CORE_DEBUG_ASSERT(allocator)

    allocator->FreeComponent(component);
}

Entity* EntityRegistry::CreateEntity()
{
	util::Hasher hasher;
    hasher.u64(cookie_++);
	auto* entity = entityPool_.allocate(this, hasher.get());
	entity->EntityRegistryOffset_ = entities_.size();
	entities_.push_back(entity);
	return entity;
}

void EntityRegistry::DeleteEntity(Entity *entity)
{
    // Delete all components of entity
    {
        auto& list = entity->componentMap_.inner_list();
        auto itr = list.begin();
        while (itr != list.end()) {
            itr = list.erase(itr);
            UnRegister(entity, itr.get()->get());
        }
    }

	auto offset = entity->EntityRegistryOffset_;
	CORE_DEBUG_ASSERT(offset < entities_.size());

	entities_[offset] = entities_.back();
	entities_[offset]->EntityRegistryOffset_ = offset;
	entities_.pop_back();
	entityPool_.free(entity);
}

void EntityRegistry::GroupKeySet::insert(ComponentType type)
{
    set.emplace_yield(type);
}

EntityRegistry::~EntityRegistry()
{
    // delete manually allocated component allocator
    {
		auto &list = componentAllocators_.inner_list();
		auto itr = list.begin();
		while (itr != list.end())
		{
			auto *to_free = itr.get();
			itr = list.erase(itr);
			delete to_free;
		}
        componentAllocators_.clear();
	}

    // delete manully allocated entity group
    {
        auto &list = entityGroups_.inner_list();
        auto itr = list.begin();
        while (itr != list.end())
        {
            auto *to_free = itr.get();
            itr = list.erase(itr);
            delete to_free;
        }
        entityGroups_.clear();
    }

}

}