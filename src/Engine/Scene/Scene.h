#pragma once
#include "GameObject/Components/MeshCmpt.h"
#include "Graphics/Vulkan/Descriptor.h"
#include "GameObject/GameObject.h"
#include "GameObject/Components/CameraCmpt.h"
#include "Graphics/Vulkan/Buffer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/DrawContext.h"


class Scene {

public:
    std::string name;
    
    Scene(const std::string& name);
    ~Scene();

    GameObject* AddGameObject(const std::string& name, GameObject* parent = nullptr);
    void SetMainCamera(CameraCmpt* camera) { mainCamera_ = camera; }

    CameraCmpt* GetMainCamera() { return mainCamera_; }

    template<typename T>
    std::vector<T*> GetComponents();

private:
    // game objects map
    std::vector<std::unique_ptr<GameObject>> gameObjects_;
    
    // nodes that dont have a parent, for iterating through the file in tree order
    std::unique_ptr<GameObject> root_;

    // cameras
    // std::vector<CameraCmpt*> cameras_;
    CameraCmpt* mainCamera_;

    DrawContext drawContext_;
};

template<typename T>
std::vector<T*> Scene::GetComponents()
{
    std::vector<T*> components;
    components.reserve(gameObjects_.size());
    for (auto& obj : gameObjects_) {
        if (auto comp = obj->GetComponent<T>()) 
            components.push_back(comp);
    }

    return components;
}