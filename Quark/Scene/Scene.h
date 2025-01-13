#pragma once
#include "Quark/Ecs/EntityRegistry.h"
#include "Quark/Core/UUID.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Asset/Asset.h"

#include <glm/glm.hpp>

#include <string>

namespace quark {

struct CameraCmpt;
struct Texture;
struct ArmatureCmpt;
struct SkeletonAsset;

class Scene {
public:
    std::string sceneName = "Unnamed";

public:
    Scene(const std::string& name);
    ~Scene();

    void OnUpdate(TimeStep delta_time);

    // per-frame updating systems
    // void RunTransformUpdateSystem();
    void RunAnimationUpdateSystem(TimeStep delta_time);
    void RunJointsUpdateSystem();

    // swap data for rendering
    void FillMeshSwapData();
    void FillCameraSwapData();
    
    // entity
    Entity* CreateEntity(const std::string& name = "", Entity* parent = nullptr);
    Entity* CreateEntityWithID(UUID id, const std::string& name = "", Entity* parent = nullptr);
    Entity* GetEntityWithID(UUID id);
    Entity* GetEntityWithName(const std::string& name);
    Entity* GetParentEntity(Entity* entity);
    std::vector<Entity*> GetChildEntities(Entity* parent, bool recursive);

    void DeleteEntity(Entity* entity);
    void AddArmatureComponent(Entity* entity, Ref<SkeletonAsset> skeleton_asset); // now the root bone entity's parent is the parent of the parameter entity

    std::vector<Entity*>& GetEntities() { return m_entity_registry.GetEntities(); }

    void AttachChild(Entity* child, Entity* parent);
    void DetachChild(Entity* child);

    template<typename... Ts>
    ComponentGroupVector<Ts...>& GetComponents() 
    { 
        return m_entity_registry.GetEntityGroup<Ts...>()->GetComponentGroup(); 
    }

    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() const
    { 
        return m_entity_registry.GetEntityGroup<Ts...>()->GetComponentGroup();
    }

    template<typename... Ts>
    std::vector<Entity*>& GetAllEntitiesWith()
    {
        return m_entity_registry.GetEntityGroup<Ts...>()->GetEntities();
    }

    // Cameras
    void SetMainCameraEntity(Entity* cam) { m_main_camera_entity = cam; }
    Entity* GetMainCameraEntity();

private:
    void BuildBoneEntities(Entity*bone_intity, uint32_t bone_index, ArmatureCmpt* armature_cmpt);

    EntityRegistry m_entity_registry;
    Entity* m_main_camera_entity;
    std::unordered_map<uint64_t, Entity*> m_id_to_entity_map;
    std::unordered_map<std::string, Entity*> m_name_to_entity_map;
};

}