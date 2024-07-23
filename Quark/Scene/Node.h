#pragma once
#include "Scene/Ecs.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Components/NameCmpt.h"

namespace scene {
class Scene;

// This class composite the scene-graph topology and ECS logic.
// You can not create a node with new, you have to use the Scene::CreateNode() method.
class Node {
    friend class Scene;
public:
    Node(Scene* scene, Entity* entity, size_t poolOffset);
    ~Node() = default;

    Node* GetParent() const { return parent_; }

    void AddChild(Node* child);
    void RemoveChild(Node* child);
    void ClearChildren();
    
    std::vector<Node*>& GetChildren() { return children_;} 
    const std::vector<Node*>& GetChildren() const { return children_;}

    Entity* GetEntity() { return entity_;}
    const Entity* GetEntity() const { return entity_; }

private:
    Scene* scene_;
    Entity* entity_;    //  life managemenet here
    
    Node* parent_;
    std::vector<Node*> children_;
    size_t poolOffset_;

};

}