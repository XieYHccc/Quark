#pragma once
#include "Util/ObjectPool.h"
#include "Graphic/Common.h"
#include "Renderer/Common.h"
#include "Scene/Ecs.h"
#include "Scene/Components/Common.h"

namespace scene {
class Scene;

// This class composite the scene-graph topology and ECS.
class Node {
    friend class util::ObjectPool<Node>;
    friend class Scene;
public:
    ~Node();

    void SetParent(Node* parent) { parent_ = parent; }
    void AddChild(Node* child) {children_.push_back(child); }

    Entity* GetEntity() { return entity_;}
    const Entity* GetEntity() const {return entity_;}
    bool HasEntity() { return entity_;}

private:
    Node(Scene* scene, Node* parent = nullptr, bool create_entity = false);
    Scene* scene_;
    Node* parent_;
    std::vector<Node*> children_;
    Entity* entity_;    //  life managemenet here
};

class CameraCmpt;
class Scene {
    friend class Node;
public:
    Scene(const std::string& name);
    ~Scene();

    /*** Node Hierachy ***/
    Node* CreateNode(const std::string& name, Node* parent = nullptr);
    Node* CreateNode(Node* parent = nullptr, bool create_entity = false);
    void DeleteNode(Node* node);
    Node* GetNodeFromName(const std::string& name);

    template<typename... Ts>
    auto GetComponents() { registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    void SetCamera(CameraCmpt* cam) { mainCamera_ = cam; }

private:
    std::string name_;
    Node* root_;
    CameraCmpt* mainCamera_;
    EntityRegistry registry_;
    std::unordered_map<std::string, Node*> nodeMap_; //TODO: Hash the node
    util::ObjectPool<Node> nodePool_;

    // Resources
    std::vector<Ref<graphic::Image>> images_;
    std::vector<Ref<graphic::Sampler>> samplers_;
    std::vector<Ref<render::Mesh>> meshes_;
    std::vector<Ref<render::Material>> materials_;
};
}