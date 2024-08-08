#include "Quark/QuarkPch.h"
#include "Quark/Scene/GameObject.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CommonCmpts.h"

namespace quark {
GameObject::GameObject(Scene* scene, Entity* entity, size_t poolOffset)
    :m_Scene(scene), m_Entity(entity), m_PoolOffset(poolOffset)
{

}

std::vector<GameObject*>& GameObject::GetChildren() 
{
    auto* relationshipCmpt = m_Entity->GetComponent<RelationshipCmpt>();
    return relationshipCmpt->children;
}  

const std::vector<GameObject*>& GameObject::GetChildren() const 
{
    const auto* relationshipCmpt = m_Entity->GetComponent<RelationshipCmpt>();
    return relationshipCmpt->children;
}

GameObject* GameObject::GetParent() const 
{
    auto* relationshipCmpt = m_Entity->GetComponent<RelationshipCmpt>();
    return relationshipCmpt->parent;
}

void GameObject::RemoveChild(GameObject* child)
{
    CORE_DEBUG_ASSERT(child != nullptr)

    if (child->GetParent() != this) 
    {
        CORE_LOGW("GameObject::RemoveChild()::You can't remove a child which is not your child.")
        return;     
    }

    std::vector<GameObject*>& children = GetChildren();
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
        return;
    }

    CORE_LOGW("GameObject::RemoveChild()::You can't remove a child which is not your child.")
    
}

void GameObject::ClearChildren()
{
    for (auto* c: GetChildren())
        RemoveChild(c);
}

void GameObject::AddChild(GameObject *child)
{
    CORE_DEBUG_ASSERT(child != nullptr)
    CORE_DEBUG_ASSERT(child != this)

    auto* childRelationship = child->GetEntity()->GetComponent<RelationshipCmpt>();
    auto* thisRelationship = m_Entity->GetComponent<RelationshipCmpt>();

    if (childRelationship->parent != nullptr) 
    {
        CORE_LOGW("GameObject::AddChild()::You can't add a child which has a parent.")
        return;
    }

    for (const auto* c : thisRelationship->children) 
    {
        if (c == child) 
        {
            CORE_LOGW("GameObject::AddChild()::You can't add a child which has existed.")
            return;       
        }
    }

    childRelationship->parent = this;
    thisRelationship->children.push_back(child);
}

}