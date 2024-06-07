#pragma once
#include "Rendering/Vulkan/VulkanAssert.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class RenderDevice_Vulkan;
// This class is responsible for selecting physical device，creating logical device and allocator
class VulkanDevice {
public:
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VmaAllocator vmaAllocator;
    VkQueue graphicQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkQueue computeQueue;
    u32 graphicQueueIndex;
    u32 presentQueueIndex;
    u32 transferQueueIndex;
    u32 computeQueueIndex;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

public:
    VulkanDevice(RenderDevice_Vulkan& driver);
    ~VulkanDevice();

    SwapChainSupportDetails GetSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkFormat GetDepthFormat() const;

private:
    void PickGPU(VkInstance instance, VkSurfaceKHR surface);
    void CreateLogicalDevice();
    void CreateAllocator();
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;

    struct QueueFamilyIndices {
        u32 present = UINT32_MAX;
        u32 graphics = UINT32_MAX;
        u32 transfer = UINT32_MAX;
        u32 compute = UINT32_MAX;
        bool isComplete() const { return present != UINT32_MAX && graphics != UINT32_MAX && compute != UINT32_MAX && transfer != UINT32_MAX; }
    };
    QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) const;

private:
    RenderDevice_Vulkan& driver_;
};