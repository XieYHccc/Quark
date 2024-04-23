#pragma once

#include "GameObject/Components/MeshRendererCmpt/MeshRenderCmpt.h"
#include "Graphics/Vulkan/DescriptorVulkan.h"
#include "pch.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "GameObject/GameObject.h"
#include "GameObject/Camera.h"
#include "GameObject/Components/CameraCmpt/CameraCmpt.h"



class Scene {
public:
    // gpu resources 
    std::unordered_map<std::string, std::unique_ptr<GpuMeshVulkan>> meshesMap;
    std::unordered_map<std::string, GpuImageVulkan> imagesMap;
    std::unordered_map<std::string, std::unique_ptr<GpuMaterialInstance>> materialsMap;
    std::vector<VkSampler> samplers;
    DescriptorAllocator descriptorPool;
    GpuBufferVulkan materialDataBuffer;

public:
    Scene();
    ~Scene() {};

    void Update();
    void ClearResourses();

    GameObject* AddGameObject(const std::string& name, GameObject* parent = nullptr);
    void SetMainCamera(CameraCmpt* camera) { mainCamera_ = camera; }

    CameraCmpt* GetMainCamera() { return mainCamera_; }
    const DrawContext& GetDrawContext() const { return drawContext_; }

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