#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <typeindex>

#include "GameObject/Components/TransformCmpt/TransformCmpt.h"

class GameObject {

public:
    GameObject* parent;
    std::vector<GameObject*> childrens;
    TransformCmpt* transformCmpt;

public:
    GameObject() = delete;
    GameObject(const std::string& name, GameObject* parentObject = nullptr)
        :name_(name), parent(parentObject)
    {
        transformCmpt = AddComponent<TransformCmpt>();
    } 

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
    inline T* AddComponent() 
    {
        auto cmpt = std::make_unique<T>(this);
        cmpt->Awake();

        // move the ownership
        auto base = static_cast<Component*>(cmpt.release());
        components_.emplace(T::GetStaticType(), base);

        return static_cast<T*>(base);
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
    inline T* GetComponent() 
    {
        auto it = components_.find(T::GetStaticType());
        if (it == components_.end()) 
            return static_cast<T*>(nullptr);

        return static_cast<T*>(it->second.get());
    }
    
private:
    std::string name_;
    std::multimap<std::uint32_t, std::unique_ptr<Component>> components_;

};