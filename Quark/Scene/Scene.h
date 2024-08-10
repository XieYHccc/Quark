#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Quark/Core/Util/ObjectPool.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Ecs/EntityRegistry.h"
#include "Quark/Scene/GameObject.h"
#include "Quark/Scene/Resources/Texture.h"
#include "Quark/Scene/Resources/Material.h"
#include "Quark/Scene/Resources/Mesh.h"

namespace quark {

class CameraCmpt;
class Scene {
    friend class GLTFLoader;
    friend class MeshLoader;
    friend class GameObject;
public:
    Scene(const std::string& name);
    ~Scene();

    void Update();

    /*** GameObject ***/
    GameObject* CreateGameObject(const std::string& name = "Null", GameObject* parent = nullptr);
    void DeleteGameObject(GameObject* GameObject);
    // GameObject* GetGameObjectByName(const std::string& name);
    GameObject* GetRootGameObject() { return m_Root;}

    /*** Transform Tree ***/
    void UpdateTransformTree(GameObject* GameObject, const glm::mat4& mat);

    /***   Components   ***/
    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() { return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup(); }
    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const { return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    /***    Cameras    ***/
    void SetCamera(GameObject* cam) { m_CameraObject = cam; }
    CameraCmpt* GetCamera();

    void SetName(const std::string& name);
private:
    EntityRegistry m_Registry;
    std::vector<GameObject*> m_GameObjects;

    GameObject* m_Root;
    GameObject* m_CameraObject;

    //std::unordered_map<std::string, GameObject*> GameObjectMap_; TODO: Support name search
    util::ObjectPool<GameObject> m_GameObjectPool;

};

}

#include "Quark/Scene/GameObjectTemplates.inl"