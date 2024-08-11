#include "Quark/QuarkPch.h"
#include "Quark/Ecs/Entity.h"
#include "Quark/Ecs/EntityRegistry.h"
namespace quark {

void EntityRegistry::UnRegister(Entity* entity, Component* component)
{
    if (component == nullptr)
        return;

    auto id = component->GetType();
    entity->m_ComponentMap.erase(id);

    auto* group_key_set = m_ComponentToGroups.find(id);
    if (group_key_set) {
        for (auto &group_key : *group_key_set) {
            auto* group = m_EntityGroups.find(group_key.get_hash());
            if (group)
                group->RemoveEntity(*entity);
        }
    }  

    auto* allocator = m_ComponentAllocators.find(id);
    CORE_DEBUG_ASSERT(allocator)

    allocator->FreeComponent(component);
}

Entity* EntityRegistry::CreateEntity()
{
	util::Hasher hasher;
    hasher.u64(m_Cookie++);
	auto* entity = m_EntityPool.allocate(this, hasher.get());
	entity->m_OffsetInRegistry = m_Entities.size();
	m_Entities.push_back(entity);
	return entity;
}

void EntityRegistry::DeleteEntity(Entity *entity)
{
    // Delete all components of entity
    {
        auto& list = entity->m_ComponentMap.inner_list();
        auto itr = list.begin();
        while (itr != list.end()) {
            UnRegister(entity, itr.get()->get());
            itr = list.erase(itr);
        }
    }

	auto offset = entity->m_OffsetInRegistry;
	CORE_DEBUG_ASSERT(offset < m_Entities.size());

	m_Entities[offset] = m_Entities.back();
	m_Entities[offset]->m_OffsetInRegistry = offset;
	m_Entities.pop_back();
	m_EntityPool.free(entity);
}

void EntityRegistry::GroupKeySet::insert(ComponentType type)
{
    set.emplace_yield(type);
}

EntityRegistry::~EntityRegistry()
{
    // Delete all entities
    for (auto e : m_Entities) {
        DeleteEntity(e);
    }
    
    // Delete manually allocated component allocator
    {
		auto &list = m_ComponentAllocators.inner_list();
		auto itr = list.begin();
		while (itr != list.end())
		{
			auto *to_free = itr.get();
			itr = list.erase(itr);
			delete to_free;
		}
        m_ComponentAllocators.clear();
	}

    // Delete manully allocated entity group
    {
        auto &list = m_EntityGroups.inner_list();
        auto itr = list.begin();
        while (itr != list.end())
        {
            auto *to_free = itr.get();
            itr = list.erase(itr);
            delete to_free;
        }
        m_EntityGroups.clear();
    }

}

}