#include <Graphics/Vulkan/Image.h>
#include <Application/Application.h>
#include <Scene/Scene.h>
#include "Renderer/Passes/GeometryPass.h"

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