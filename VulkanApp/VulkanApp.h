#pragma once
#include <string>
#include <Engine/Graphics/Vulkan/Image.h>
#include "Engine/Application/Application.h"
#include <Engine/Scene/Scene.h>
#include "Engine/Renderer/Passes/GeometryPass.h"

class VulkanApp : public Application 
{  
public:
    VulkanApp(const std::string& title, const std::string& root, int width, int height);

    ~VulkanApp();
    
    void Update() override;
    void Render() override;

    Scene* scene;

    float yaw;
    float pitch;

    vk::Image colorAttachment;
    vk::Image depthAttachment;
    VkExtent2D drawExtent;

    std::unique_ptr<GeometryPass> geometryPass;


};