#pragma once
#include "Graphic/Vulkan/Common_Vulkan.h"

namespace graphic {

struct SwapChainSupportDetail {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct physicalDeviceRequirement
{
    std::vector<const char*> deviceExtensions;
    bool samplerAnisotropy  = true;
    bool ForceDescreteGpu = true;
    bool preferDescreteGPU = true;
};

/* Responsibilities
// 1. Create VkInstance, VkDebugMessenger(in debug build)
// 2. Check, select a VkPhysicalDevice and store the supported features and extensions
// 3. Create VkDevice and get VkQueue (Graphics, Async compute, Async transfer)
// 4. Get the address of extend functions
// 5. Create a allocator for resouce creation
// 6. Create a VkSwapchain for presenting
*/
class VulkanContext {
public:
    VkInstance instance = VK_NULL_HANDLE;
#ifdef QK_DEBUG_BUILD
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
    VmaAllocator vmaAllocator = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    std::vector<VkImage> swapChianImages;
    std::vector<VkImageView> swapChainImageViews;
    VkExtent2D swapChainExtent = {};

    VkQueue graphicQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    u32 graphicQueueIndex = UINT32_MAX;
    u32 transferQueueIndex = UINT32_MAX;
    u32 computeQueueIndex = UINT32_MAX;
    std::vector<uint32_t> uniqueQueueFamilies;

    VkPhysicalDeviceMemoryProperties memoryPorps = {};
    VkPhysicalDeviceProperties2 properties2 = {};
    VkPhysicalDeviceVulkan11Properties properties11 = {};
    VkPhysicalDeviceVulkan12Properties properties12 = {};
    VkPhysicalDeviceVulkan13Properties properties13 = {};
    VkPhysicalDeviceFeatures2 features2 = {};
    VkPhysicalDeviceVulkan11Features features11 = {};
    VkPhysicalDeviceVulkan12Features features12 = {};
    VkPhysicalDeviceVulkan13Features features13 = {};
    std::vector<const char*> enabledExtensions;

    struct ExtendFunction {
        PFN_vkQueueSubmit2KHR pVkQueueSubmit2KHR;
        PFN_vkCmdBlitImage2KHR pVkCmdBlitImage2KHR;
        PFN_vkCmdPipelineBarrier2KHR pVkCmdPipelineBarrier2KHR;
        PFN_vkCmdBeginRenderingKHR pVkCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR pVkCmdEndRenderingKHR;
    } extendFunction;

    VulkanContext();
    ~VulkanContext();

	void CreateSwapChain();
    void DestroySwapChain();
    
private:
    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateVmaAllocator();
    void InitExtendFunctions();

    std::vector<const char*> GetRequiredExtensions() const;
    SwapChainSupportDetail GetSwapchainSupportDetails() const;

    bool IsPhysicalDeviceSuitable(VkPhysicalDevice device, physicalDeviceRequirement& requirements);
    // Debug callback function，VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
    static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};
}