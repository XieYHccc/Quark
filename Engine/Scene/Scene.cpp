#include "pch.h"
#include "Util/Hash.h"
#include "Scene/Scene.h"
#include "Scene/Components/NameCmpt.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Components/CameraCmpt.h"

namespace scene {
Node::Node(Scene* scene, Node* parent, const std::string& name)
    :scene_(scene), parent_(parent)
{
    if (parent) {
        parent->AddChild(this);
    }

    entity_ = scene_->registry_.CreateEntity();
    transformCmpt_ = entity_->AddComponent<TransformCmpt>(this);

    if (name != "Null") {
        nameCmpt_ = entity_->AddComponent<NameCmpt>();
        nameCmpt_->name = name;
    }
}

Node::~Node()
{
    if (entity_)
         scene_->registry_.DeleteEntity(entity_); 
}

Scene::Scene(const std::string& name)
    : name_(name)
{
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
    if (nodeMap_.find(name) != nodeMap_.end()) {
        CORE_LOGW("You can't create a node with a name which has existed.")
        return nullptr;;
    }

    Node* newNode = nodePool_.allocate(this, parent, name);
    nodes_.push_back(newNode);
    nodeMap_.emplace(std::make_pair(name, newNode));
    
    if (parent == nullptr)
        rootNodes_.push_back(newNode);
    else {
        parent->AddChild(newNode);
        newNode->SetParent(parent);
    }

    return newNode;
    
}


void Scene::DeleteNode(Node *node)
{

    // TODO: Support node deletion
    
    // Iteratively delete children nodes
    for (auto* child : node->children_) {
        DeleteNode(child);
    }

    // If node's entity has name, erase it from map
    auto name_cmpt = node->GetNameCmpt();
    if (name_cmpt)
        nodeMap_.erase(name_cmpt->name);
    

    // free the node in pool
    nodePool_.free(node);
}

Node* Scene::GetNodeByName(const std::string &name)
{
    auto find = nodeMap_.find(name);

    if (find != nodeMap_.end())
        return find->second;
    else {
        CORE_LOGD("Node named {} doesn't exsit.")
        return nullptr;
    }
}

void Scene::FlushRootNodes()
{
    rootNodes_.clear();
    for (auto* node : nodes_) {
        if (node->parent_ == nullptr) {
            rootNodes_.push_back(node);
        }
    }
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
