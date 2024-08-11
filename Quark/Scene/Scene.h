#pragma once
#include <string>
#include <glm/glm.hpp>

#include "Quark/Core/Util/ObjectPool.h"
#include "Quark/Core/UUID.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Ecs/EntityRegistry.h"
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

    void SetSceneName(const std::string& name);
    void Update();

    Entity* CreateEntity(const std::string& name = "", Entity* parent = nullptr);
    Entity* CreateEntityWithID(UUID idconst, const std::string& name = "", Entity* parent = nullptr);
    Entity* GetRootEntity() { return m_RootEntity; }
    void DeleteEntity(Entity* entity);

    /***   Components   ***/
    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() { return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const { return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup(); }

    /***    Cameras    ***/
    void SetCameraEntity(Entity* cam) { m_CameraEntity = cam; }
    Entity* GetCameraEntity();

private:
    EntityRegistry m_Registry;

    Entity* m_RootEntity;
    Entity* m_CameraEntity;
    
    std::unordered_map<UUID, Entity*> m_EntityIdMap;

};

}