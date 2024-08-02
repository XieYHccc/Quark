#pragma once
#include <list>
#include <glm/glm.hpp>
#include "Util/ObjectPool.h"
#include "Graphic/Common.h"
#include "Scene/Ecs.h"
#include "Scene/Node.h"
#include "Scene/Resources/Texture.h"
#include "Scene/Resources/Material.h"
#include "Scene/Resources/Mesh.h"

namespace asset {
class GLTFLoader;
class MeshLoader;
}

namespace scene {

class CameraCmpt;
class Scene {
    friend class asset::GLTFLoader;
    friend class asset::MeshLoader;
    friend class Node;
public:
    Scene(const std::string& name);
    ~Scene();

    void Update();

    /*** Node Hierachy ***/
    Node* CreateNode(const std::string& name = "Null", Node* parent = nullptr);
    // Node* GetNodeByName(const std::string& name);
    Node* GetRootNode() { return rootNode_;}
    void DeleteNode(Node* node);

    /*** Transform Tree ***/
    void UpdateTransformTree(Node* node, const glm::mat4& mat);

    /***   Components   ***/
    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }
    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    /***    Cameras    ***/
    void SetCamera(Node* cam) { cameraNode_ = cam; }
    CameraCmpt* GetCamera();

    void SetName(const std::string& name);
private:
    EntityRegistry registry_;

    // Nodes
    std::vector<Node*> nodes_;
    Node* rootNode_;
    Node* cameraNode_;
    //std::unordered_map<std::string, Node*> nodeMap_; TODO: Support name search
    util::ObjectPool<Node> nodePool_;

};
}