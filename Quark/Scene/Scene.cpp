#include "pch.h"
#include "Util/Hash.h"
#include "Scene/Scene.h"
#include "Scene/Components/NameCmpt.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Components/CameraCmpt.h"

namespace scene {

Scene::Scene(const std::string& name)
{
    // Create root node
    Entity* rootEntity = registry_.CreateEntity();
    rootNode_ = nodePool_.allocate(this, rootEntity, 0);

    rootEntity->AddComponent<NameCmpt>(name);
    rootEntity->AddComponent<TransformCmpt>(rootNode_);

    nodes_.push_back(rootNode_);
}

Scene::~Scene()
{   
    // Free all nodes
    for (auto node : nodes_) {
        nodePool_.free(node);
    }

}

Node* Scene::CreateNode(const std::string &name, Node* parent)
{
    // Create entity
    Entity* newEntity = registry_.CreateEntity();

    // Has name?
    if (name != "Null") {
        newEntity->AddComponent<NameCmpt>(name);
    }

    // Create node
    size_t offset = nodes_.size();
    Node* newNode = nodePool_.allocate(this, newEntity, offset);
    nodes_.push_back(newNode);

    // Create transform component
    newEntity->AddComponent<TransformCmpt>(newNode);

    // Node's hierarchy 
    if (parent != nullptr)
        parent->AddChild(newNode);

    return newNode;
    
}

void Scene::SetName(const std::string &name)
{
    rootNode_->GetEntity()->GetComponent<NameCmpt>()->name = name;
}

void Scene::DeleteNode(Node *node)
{

    // Remove from parent
    if (node->GetParent()) {
        node->GetParent()->RemoveChild(node);
        node->parent_ = nullptr;
    }

    // Delete entity
    registry_.DeleteEntity(node->GetEntity());

    // Delete children
    for (auto* c: node->GetChildren()) {
        c->parent_ = nullptr; // So, the children vector will not be modified
        DeleteNode(c);
    }

    // free the node in pool
    nodes_[node->poolOffset_] = nodes_.back();
    nodes_[node->poolOffset_]->poolOffset_ = node->poolOffset_;
    nodes_.pop_back();
    nodePool_.free(node);
}

CameraCmpt* Scene::GetCamera()
{
    if (cameraNode_) {
        return cameraNode_->GetEntity()->GetComponent<CameraCmpt>();
    }
    else {
        CORE_LOGW("Scene doesn't have a camera, but you are requesting one.")
        return nullptr;
    }
}

void Scene::Update()
{

}

}
