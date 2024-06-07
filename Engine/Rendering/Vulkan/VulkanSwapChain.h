#pragma once
#include "Rendering/Vulkan/VulkanAssert.h"
#include "Rendering/RenderTypes.h"

class RenderDevice_Vulkan;

// This class is responsible for talking to our window
class VulkanSwapChain {
public:
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D swapChainExtent;
    GPUImage depthimage;
    std::vector<GPUImage> swapChainImages;
    DataFormat imageFormat;

    VulkanSwapChain(RenderDevice_Vulkan& driver, u32 width, u32 height);
    ~VulkanSwapChain();

    void Resize(u32 width, u32 height);
    void Present(VkQueue graphicQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 imgeIndex);
    bool AquireNextImageIndex(VkSemaphore imageAvailableSemaphore, VkFence fence, u64 timeoutNs, u32* outImageIndex);

private:
    void CreateSwapChain(u32 width, u32 height);
    void DestroySwapChain();

    RenderDevice_Vulkan& driver_;

};
