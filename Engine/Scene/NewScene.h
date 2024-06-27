#pragma once
#include "Util/CompileTimeHash.h"
#include "Scene/Entity.h"

namespace scene {
class CameraCmpt;
class Scene {
public:
    Scene(const std::string& name);
    ~Scene();

    Entity* AddEntity(const std::string& name, Entity* parent = nullptr);
    void DeleteEntity(Entity* entity);

    template<typename... Ts>
    const ComponentGroupVector<Ts...>& GetComponents() {
        registry_.GetEntityGroup<Ts...>().GetComponentGroup();
    }

    template<typename... Ts>
    const std::vector<Entity*>& GetEntitys() {
        registry_.GetEntityGroup<Ts...>().GetEntities();
    }

private:
    std::string name_;
    Entity* root_;
    CameraCmpt* mainCamera_;
    EntityRegistry registry_;
};
}