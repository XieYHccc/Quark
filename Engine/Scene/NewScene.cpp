#include "pch.h"
#include "Util/Hash.h"
#include "Scene/NewScene.h"
#include "Scene/Components/Common.h"
namespace scene {

Node::Node(Scene* scene, Node* parent, bool create_entity)
    :scene_(scene), parent_(parent)
{
    if (parent) {
        parent->AddChild(this);
    }
    
    if (create_entity)
        entity_ = scene_->registry_.CreateEntity();
}

Node::~Node()
{
    if (entity_)
        scene_->registry_.DeleteEntity(entity_); 
}

Scene::Scene(const std::string& name)
    : name_(name)
{
    root_ = CreateNode("Root");
}

Scene::~Scene()
{

}

Node* Scene::CreateNode(const std::string &name, Node* parent)
{
    if (nodeMap_.find(name) != nodeMap_.end()) {
        CORE_LOGW("You can't create a node with a name which has existed.")
        return nullptr;;
    }

    Node* newNode = CreateNode(parent, true);
    newNode->GetEntity()->AddComponent<NameCmpt>()->name = name;
    nodeMap_.emplace(std::make_pair(name, newNode));
    
    return newNode;
    
}

Node* Scene::CreateNode(Node* parent, bool create_entity)
{
   return nodePool_.allocate(this, parent, create_entity);
}

void Scene::DeleteNode(Node *node)
{
    // Iteratively delete children nodes
    for (auto* child : node->children_) {
        DeleteNode(child);
    }

    // If node's entity has name, erase it from map
    if (node->HasEntity()) {
        auto* name_cmpt = node->GetEntity()->GetComponent<NameCmpt>();
        if (name_cmpt)
            nodeMap_.erase(name_cmpt->name);
    }

    // free the node in pool
    nodePool_.free(node);
}

Node* Scene::GetNodeFromName(const std::string &name)
{
    auto find = nodeMap_.find(name);

    if (find != nodeMap_.end())
        return find->second;
    else {
        CORE_LOGD("Node : {} doesn't exsit.")
        return nullptr;
    }
}

}