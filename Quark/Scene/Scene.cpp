#include "Quark/QuarkPch.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"

namespace quark {

Scene::Scene(const std::string& name)
{
    // Create root GameObject
    m_Root = CreateGameObject(name, nullptr);
}

Scene::~Scene()
{   
    // Free all GameObjects
    for (auto obj : m_GameObjects) {
        m_GameObjectPool.free(obj);
    }

}

GameObject* Scene::CreateGameObject(const std::string &name, GameObject* parent)
{
    // Create entity
    Entity* newEntity = m_Registry.CreateEntity();

    // Create GameObject
    size_t offset = m_GameObjects.size();
    GameObject* newGameObject = m_GameObjectPool.allocate(this, newEntity, offset);
    m_GameObjects.push_back(newGameObject);

    // Default components
    newEntity->AddComponent<TransformCmpt>();
    newEntity->AddComponent<RelationshipCmpt>();
    if (name != "Null") 
        newEntity->AddComponent<NameCmpt>(name);

    // GameObject's hierarchy 
    if (parent != nullptr)
        parent->AddChild(newGameObject);

    return newGameObject;
    
}

void Scene::SetName(const std::string &name)
{
    m_Root->GetEntity()->GetComponent<NameCmpt>()->name = name;
}

void Scene::DeleteGameObject(GameObject *obj)
{
    // Remove from parent
    if (obj->GetParent())
        obj->GetParent()->AddChild(obj);

    // Iteratively delete children
    for (auto* c: obj->GetChildren())
        DeleteGameObject(c);

    // Delete entity
    m_Registry.DeleteEntity(obj->GetEntity());

    // free the GameObject in pool
    m_GameObjects[obj->m_PoolOffset] = m_GameObjects.back();
    m_GameObjects[obj->m_PoolOffset]->m_PoolOffset = obj->m_PoolOffset;
    m_GameObjects.pop_back();
    m_GameObjectPool.free(obj);
}

CameraCmpt* Scene::GetCamera()
{
    if (m_CameraObject)
    {
        return m_CameraObject->GetEntity()->GetComponent<CameraCmpt>();
    }
    else 
    {
        CORE_LOGW("Scene doesn't have a camera, but you are requesting one.")
        return nullptr;
    }
}

void Scene::Update()
{

}


}