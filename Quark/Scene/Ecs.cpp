#include "qkpch.h"
#include "Scene/Ecs.h"

namespace scene {

Entity::Entity(EntityRegistry* entityRegistry, util::Hash hash)
    : entityRegistry_(entityRegistry), hashId_(hash)
{
}

void EntityRegistry::UnRegister(Entity* entity, Component* component)
{
    if (component == nullptr)
        return;

    CORE_DEBUG_ASSERT(component->GetEntity() == entity) // Double check

    auto id = component->GetType();
    entity->componentMap_.erase(id);

    auto* group_key_set = componentToGroups_.find(id);
    if (group_key_set) {
        for (auto &group_key : *group_key_set) {
            auto* group = entityGroups_.find(group_key.get_hash());
            if (group)
                group->RemoveEntity(*entity);
        }
    }  

    auto* allocator = componentAllocators_.find(id);
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
            UnRegister(entity, itr.get()->get());
            itr = list.erase(itr);
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
    // Delete all entities
    for (auto e : entities_) {
        DeleteEntity(e);
    }
    
    // Delete manually allocated component allocator
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

    // Delete manully allocated entity group
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