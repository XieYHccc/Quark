#pragma once
#include "Quark/Ecs/Entity.h"

namespace quark {

class Scene;

// A wrapper around Entity to give an Entity some extra functionalities
// and some default components
class GameObject {
    friend class Scene;
public:
    GameObject(Scene* scene, Entity* entity, size_t poolOffset);
    ~GameObject() = default;

    GameObject* GetParent() const;

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void ClearChildren();
    
    std::vector<GameObject*>& GetChildren();
    const std::vector<GameObject*>& GetChildren() const;

    Entity* GetEntity() { return m_Entity;}
    const Entity* GetEntity() const { return m_Entity; }

private:
    Scene* m_Scene;
    Entity* m_Entity;    //  life managemenet here
    size_t m_PoolOffset;

};
}