#pragma once
#include "Rendering/Vulkan/VulkanTypes.h"
#include "Rendering/RenderTypes.h"

class RenderDeviceDriver_Vulkan;

// This class is responsible for talking to our window
class VulkanSwapChain {
public:
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    u8 maxFramesInFlight;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D swapChainExtent;
    GPUImage depthimage;

    VulkanSwapChain(RenderDeviceDriver_Vulkan& driver, u32 width, u32 height);
    ~VulkanSwapChain();

    void Resize(u32 width, u32 height);
    void Present(VkQueue graphicQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 imgeIndex);
    bool AquireNextImageIndex(VkSemaphore imageAvailableSemaphore, VkFence fence, u64 timeoutNs, u32 outImageIndex);

private:
    void CreateSwapChain(u32 width, u32 height);
    void DestroySwapChain();

    RenderDeviceDriver_Vulkan& driver_;

};
