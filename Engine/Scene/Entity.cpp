#include "pch.h"
#include "Scene/Entity.h"

namespace scene {

Entity::Entity(EntityRegistry* registry, util::Hash hash, const std::string& name)
    : registry_(registry), hashId_(hash), name_(name)
{
    transformCmpt_ = AddComponent<TransformCmpt>(this);
}

void EntityRegistry::UnRegister(Entity * entity, Component* component)
{
    auto* allocator = componentAllocators_.find(component->GetType());
    CORE_DEBUG_ASSERT(allocator)
    CORE_DEBUG_ASSERT(component->GetEntity() == entity) // Double check

    auto* group_key_set = componentToGroups_.find(component->GetType());

    if (group_key_set) {
        for (auto &group_key : *group_key_set) {
            auto* group = entityGroups_.find(group_key.get_hash());
            if (group)
                group->RemoveEntity(entity);
        }
    }  

    allocator->FreeComponent(component);
}

Entity* EntityRegistry::CreateEntity(const std::string& name)
{
	util::Hasher hasher;
    hasher.u64(cookie_++);
	auto* entity = entityPool_.allocate(this, hasher.get(), name);
	entity->registryOffset_ = entities_.size();
	entities_.push_back(entity);
	return entity;
}

void EntityRegistry::DeleteEntity(Entity *entity)
{
    // Delete all components of entity
    auto& components = entity->GetComponentsMap();
    for (auto& [key, value] : components)
        UnRegister(entity, value);

	auto offset = entity->registryOffset_;
	CORE_DEBUG_ASSERT(offset < entities_.size());

	entities_[offset] = entities_.back();
	entities_[offset]->registryOffset_ = offset;
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