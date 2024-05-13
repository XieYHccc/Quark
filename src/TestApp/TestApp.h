#pragma once
#include <Graphics/Vulkan/VulkanContext.h>
#include <Graphics/Vulkan/MaterialVulkan.h>
#include <Application/Application.h>
#include <Scene/Scene.h>

class TestApp : public Application 
{  
public:
    using Application::Application;

    ~TestApp();
    
    void Update() override;
    void Render() override;

    std::shared_ptr<Scene> scene;

    vkutil::VulkanContext context;


};