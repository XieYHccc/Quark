#include "Quark/qkpch.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"

namespace quark {

void RelationshipCmpt::AddChildEntity(Entity* child) 
{
    QK_CORE_ASSERT(child != nullptr)
    QK_CORE_ASSERT(child != GetEntity())

    auto* childRelationship = child->GetComponent<RelationshipCmpt>();

    if (childRelationship->GetParentEntity() != nullptr) 
    {
        QK_CORE_LOGW_TAG("Scene", "RelationshipCmpt: You can't add a child which has a parent.");
        return;
    }

    for (const auto* c : GetChildEntities()) 
    {
        if (c == child) 
        {
            QK_CORE_LOGW_TAG("Scene", "RelationshipCmpt: You can't add a child which has existed.");
            return;       
        }
    }

    childRelationship->m_ParentEntity = GetEntity();
    m_ChildEntities.push_back(child);
}

void RelationshipCmpt::RemoveChildEntity(Entity* child)
{
    QK_CORE_ASSERT(child != nullptr)

    auto* childRelationship = child->GetComponent<RelationshipCmpt>();
    if (childRelationship->GetParentEntity() != GetEntity()) 
    {
        QK_CORE_LOGW_TAG("Scene", "RelationshipCmpt: You can't remove a child which is not your child.");
        return;     
    }

    std::vector<Entity*>& children = GetChildEntities();
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
        childRelationship->m_ParentEntity = nullptr;
        return;
    }

    QK_CORE_LOGW_TAG("Scene", "RelationshipCmpt: You can't remove a child which is not your child.");
}
}