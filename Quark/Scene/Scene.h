#pragma once
#include "Quark/Ecs/EntityRegistry.h"
#include "Quark/Core/UUID.h"

#include <glm/glm.hpp>

#include <string>

namespace quark {

struct CameraCmpt;
struct Texture;

class Scene {
public:
    std::string sceneName = "Unnamed";

public:
    Scene(const std::string& name);
    ~Scene();

    void OnUpdate();

    // updating Systems
    void RunTransformUpdateSystem();

    // fill swap Data
    void FillMeshSwapData();
    void FillCameraSwapData();
    
    // entity
    Entity* CreateEntity(const std::string& name = "", Entity* parent = nullptr);
    Entity* CreateEntityWithID(UUID id, const std::string& name = "", Entity* parent = nullptr);
    Entity* GetEntityWithID(UUID id);

    void DeleteEntity(Entity* entity);

    std::vector<Entity*>& GetEntities() { return m_Registry.GetEntities(); }

    void AttachChild(Entity* child, Entity* parent);
    void DetachChild(Entity* child);

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

    // Cameras
    void SetMainCameraEntity(Entity* cam) { m_MainCameraEntity = cam; }
    Entity* GetMainCameraEntity();

private:
    std::string m_SceneName;

    EntityRegistry m_Registry;
    Entity* m_MainCameraEntity;
    std::unordered_map<uint64_t, Entity*> m_EntityIdMap;

    friend class GLTFLoader;
    friend class MeshLoader;
};

}