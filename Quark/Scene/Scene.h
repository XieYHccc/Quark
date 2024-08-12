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
public:
    Scene(const std::string& name);
    ~Scene();

    void SetSceneName(const std::string& name);
    const std::string& GetSceneName() const;

    void OnUpdate();

    Entity* CreateEntity(const std::string& name = "", Entity* parent = nullptr);
    Entity* CreateEntityWithID(UUID id, const std::string& name = "", Entity* parent = nullptr);
    Entity* GetEntityWithID(UUID id);
    void DeleteEntity(Entity* entity);

    std::vector<Entity*>& GetEntities() { return m_Registry.GetEntities(); }

    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() 
    { 
        return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup(); 
    }

    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const
    { 
        return m_Registry.GetEntityGroup<Ts...>()->GetComponentGroup();
    }

    template<typename... Ts>
    std::vector<Entity*>& GetAllEntitiesWith()
    {
        return m_Registry.GetEntityGroup<Ts...>()->GetEntities();
    }

    /***    Cameras    ***/
    void SetCameraEntity(Entity* cam) { m_CameraEntity = cam; }
    Entity* GetCameraEntity();

private:
    std::string m_SceneName;
    EntityRegistry m_Registry;
    Entity* m_CameraEntity;
    std::unordered_map<UUID, Entity*> m_EntityIdMap;

    friend class GLTFLoader;
    friend class MeshLoader;
};

}