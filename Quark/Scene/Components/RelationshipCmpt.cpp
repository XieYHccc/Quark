#include "Quark/qkpch.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"

namespace quark {

void RelationshipCmpt::AddChildEntity(Entity* child) 
{
    CORE_DEBUG_ASSERT(child != nullptr)
    CORE_DEBUG_ASSERT(child != GetEntity())

    auto* childRelationship = child->GetComponent<RelationshipCmpt>();

    if (childRelationship->GetParentEntity() != nullptr) 
    {
        CORE_LOGW("GameObject::AddChild()::You can't add a child which has a parent.")
        return;
    }

    for (const auto* c : GetChildEntities()) 
    {
        if (c == child) 
        {
            CORE_LOGW("GameObject::AddChild()::You can't add a child which has existed.")
            return;       
        }
    }

    childRelationship->m_ParentEntity = GetEntity();
    m_ChildEntities.push_back(child);
}

void RelationshipCmpt::RemoveChildEntity(Entity* child)
{
    CORE_DEBUG_ASSERT(child != nullptr)

    auto* childRelationship = child->GetComponent<RelationshipCmpt>();
    if (childRelationship->GetParentEntity() != GetEntity()) 
    {
        CORE_LOGW("GameObject::RemoveChild()::You can't remove a child which is not your child.")
        return;     
    }

    std::vector<Entity*>& children = GetChildEntities();
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
        return;
    }

    CORE_LOGW("GameObject::RemoveChild()::You can't remove a child which is not your child.")
}
}