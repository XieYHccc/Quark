#pragma once
#include "Quark/Ecs/Entity.h"

namespace quark {

class Scene;

// 1. A wrapper around Entity to give an entity some extra functionalities and some default components
// 2. You can only create GameObject through scene
class GameObject {
public:
    GameObject(Scene* scene, Entity* entity, size_t poolOffset);
    ~GameObject() = default;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template<typename T>
    T& GetComponent();

    template<typename T>
    const T& GetComponent() const;

    template<typename T>
    bool HasComponent();

    GameObject* GetParent() const;

    void AddChild(GameObject* child);

    void RemoveChild(GameObject* child);

    void ClearChildren();

    std::vector<GameObject*>& GetChildren();
    const std::vector<GameObject*>& GetChildren() const;

    Entity* GetEntity() { return m_Entity; }
    const Entity* GetEntity() const { return m_Entity; }

private:
    Scene* m_Scene;
    Entity* m_Entity;    //  life managemenet here
    size_t m_PoolOffset;

    friend class Scene;
};

}