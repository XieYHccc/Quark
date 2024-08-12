#include "Quark/QuarkPch.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"

namespace quark {

Scene::Scene(const std::string& name)
    : m_SceneName(name), m_CameraEntity(nullptr)
{
}

Scene::~Scene()
{   
}

const std::string& Scene::GetSceneName() const
{
    return m_SceneName;
}

void Scene::DeleteEntity(Entity* entity)
{
    // Remove from parent
    auto* relationshipCmpt = entity->GetComponent<RelationshipCmpt>();
    if (relationshipCmpt->GetParentEntity())
    {
        auto* parentRelationshipCmpt = relationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->RemoveChildEntity(entity);
    }

    // Iteratively delete children
    std::vector<Entity*> children = relationshipCmpt->GetChildEntities();
    for (auto* c: children)
        DeleteEntity(c);

    // Delete entity
    m_Registry.DeleteEntity(entity);
}

Entity* Scene::CreateEntity(const std::string& name, Entity* parent)
{
    return CreateEntityWithID({}, name, parent);
}

Entity* Scene::CreateEntityWithID(UUID id, const std::string& name, Entity* parent)
{
    // Create entity
    Entity* newEntity = m_Registry.CreateEntity();

    auto* idCmpt = newEntity->AddComponent<IdCmpt>();
    auto* relationshipCmpt = newEntity->AddComponent<RelationshipCmpt>();

    newEntity->AddComponent<TransformCmpt>();
    if (!name.empty()) 
        newEntity->AddComponent<NameCmpt>(name);

    if (parent != nullptr)
    {
        auto* parentRelationshipCmpt = parent->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->AddChildEntity(newEntity);
    }

    CORE_DEBUG_ASSERT(m_EntityIdMap.find(id) == m_EntityIdMap.end())
    m_EntityIdMap[id] = newEntity;

    return newEntity;
}

Entity* Scene::GetEntityWithID(UUID id)
{
    auto find = m_EntityIdMap.find(id);
    if (find != m_EntityIdMap.end())
        return find->second;
    else
        return nullptr;
}

void Scene::SetSceneName(const std::string &name)
{
    m_SceneName = name;
}

Entity* Scene::GetCameraEntity()
{
    if (m_CameraEntity)
    {
        return m_CameraEntity;
    }
    else 
    {
        CORE_LOGW("Scene doesn't have a camera game object, but you are requesting one.")
        return nullptr;
    }
}

void Scene::OnUpdate()
{

}


}