#pragma once
#include <list>
#include <glm/glm.hpp>
#include "Quark/Core/Util/ObjectPool.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Ecs/EntityRegistry.h"
#include "Quark/Scene/GameObject.h"
#include "Quark/Scene/Resources/Texture.h"
#include "Quark/Scene/Resources/Material.h"
#include "Quark/Scene/Resources/Mesh.h"

namespace quark {

class GLTFLoader;
class MeshLoader;
class CameraCmpt;
class Scene {
    friend class GLTFLoader;
    friend class MeshLoader;
    friend class GameObject;
public:
    Scene(const std::string& name);
    ~Scene();

    void Update();

    /*** GameObject Hierachy ***/
    GameObject* CreateGameObject(const std::string& name = "Null", GameObject* parent = nullptr);
    // GameObject* GetGameObjectByName(const std::string& name);
    GameObject* GetRootGameObject() { return m_Root;}
    void DeleteGameObject(GameObject* GameObject);

    /*** Transform Tree ***/
    void UpdateTransformTree(GameObject* GameObject, const glm::mat4& mat);

    /***   Components   ***/
    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }
    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const { return registry_.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    /***    Cameras    ***/
    void SetCamera(GameObject* cam) { m_CameraObject = cam; }
    CameraCmpt* GetCamera();

    void SetName(const std::string& name);
private:
    EntityRegistry registry_;

    // GameObjects
    std::vector<GameObject*> m_GameObjects;
    GameObject* m_Root;
    GameObject* m_CameraObject;
    //std::unordered_map<std::string, GameObject*> GameObjectMap_; TODO: Support name search
    util::ObjectPool<GameObject> m_GameObjectPool;

};

}