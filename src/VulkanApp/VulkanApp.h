#include <Graphics/Vulkan/VulkanTypes.h>
#include <Graphics/Vulkan/MaterialVulkan.h>
#include <Application/Application.h>
#include <Scene/Scene.h>

class VulkanApp : public Application 
{  
public:
    using Application::Application;

    ~VulkanApp();
    
    void Update() override;
    void Render() override;

    std::shared_ptr<Scene> scene;

    float yaw;
    float pitch;

    ColorAttachment colorAttachment;
    DepthAttachment depthAttachment;
    VkExtent2D drawExtent;


};