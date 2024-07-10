#pragma once
#include <glm/glm.hpp>
#include "Util/ObjectPool.h"
#include "Graphic/Common.h"
#include "Scene/Ecs.h"
#include "Renderer/RenderTypes.h"

namespace asset {
class GLTFLoader;
}

namespace scene {
class Scene;
class TransformCmpt;
struct NameCmpt;

// This class composite the scene-graph topology and ECS.
class Node {
    friend class util::ObjectPool<Node>;
    friend class Scene;
public:
    ~Node();

    void SetParent(Node* parent) { parent_ = parent; }
    Node* GetParent() const { return parent_; }

    void AddChild(Node* child) { children_.push_back(child); }

    std::vector<Node*>& GetChildren() { return children_;} 
    const std::vector<Node*>& GetChildren() const { return children_;}

    Entity* GetEntity() { return entity_;}
    const Entity* GetEntity() const { return entity_; }

    TransformCmpt* GetTransformCmpt() const { return transformCmpt_; }
    NameCmpt* GetNameCmpt() const { return nameCmpt_;}

private:
    Node(Scene* scene, Node* parent = nullptr, const std::string& name = "Null");
    Scene* scene_;
    Node* parent_;
    std::vector<Node*> children_;
    TransformCmpt* transformCmpt_;
    NameCmpt* nameCmpt_;
    Entity* entity_;    //  life managemenet here
};

class CameraCmpt;
class Scene {
    friend class asset::GLTFLoader;
    friend class Node;
public:
    Scene(const std::string& name);
    ~Scene();

    void Update();

    /*** Node Hierachy ***/
    Node* CreateNode(const std::string& name = "Null", Node* parent = nullptr);
    void DeleteNode(Node* node);
    Node* GetNodeByName(const std::string& name);
    void FlushRootNodes();

    /*** Transform Tree ***/
    void UpdateTransformTree(Node* node, const glm::mat4& mat);

    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    void SetCamera(Node* cam) { cameraNode_ = cam; }
    CameraCmpt* GetCamera();

private:
    std::string name_;
    std::vector<Node*> rootNodes_;
    std::vector<Node*> nodes_;
    Node* cameraNode_;

    EntityRegistry registry_;
    std::unordered_map<std::string, Node*> nodeMap_;
    util::ObjectPool<Node> nodePool_;

    // Resources : stored here mostly for deserilization
    std::vector<Ref<graphic::Image>> images_;
    std::vector<Ref<graphic::Sampler>> samplers_;
    std::vector<Ref<render::Mesh>> meshes_;
    std::vector<Ref<render::Material>> materials_;
    std::vector<Ref<render::Texture>> textures_;
    Ref<graphic::Buffer> materialUniformBuffer_;
};
}