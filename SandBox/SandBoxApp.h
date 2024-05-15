#include "Engine/Graphics/Vulkan/Image.h"
#include "Engine/Renderer/Passes/GeometryPass.h"
#include <Engine/Application/Application.h>

class Scene;

class SandBoxApp : public Application 
{  
public:
    SandBoxApp(const std::string& title, const std::string& root, int width, int height);
    ~SandBoxApp();

    void Update() override {}
    void Render() override;

    Scene* scene;    
        
    float yaw;
    float pitch;

    vk::Image colorAttachment;
    vk::Image depthAttachment;
    VkExtent2D drawExtent;

    std::unique_ptr<GeometryPass> geometryPass;
};