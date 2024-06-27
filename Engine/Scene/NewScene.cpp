#include "pch.h"
#include "Scene/NewScene.h"

namespace scene {

Scene::Scene(const std::string& name)
    : name_(name)
{
    root_ = registry_.CreateEntity("Root");
}

Scene::~Scene()
{

}

Entity* Scene::AddEntity(const std::string &name, Entity* parent)
{
    Entity* newEntity = registry_.CreateEntity(name);
    if (parent != nullptr) {
        newEntity->SetParent(parent);
        parent->AddChild(newEntity);
    }
    return newEntity;
}

void Scene::DeleteEntity(Entity *entity)
{
    if (entity->GetParent()) {
        entity->GetParent()->RemoveChild(entity);
    }

    for (auto& child : entity->children_) {
        DeleteEntity(&child);
    }

    registry_.DeleteEntity(entity);
}
}