#pragma once
#include "Events/ApplicationEvent.h"
#include "Graphics/Vulkan/Assert.h"

struct GLFWwindow;

namespace vk {

struct SwapChainImage 
{
    VkImage image;
    VkImageView imageView;
};

struct QueueFamilyIndices
{
    uint32_t present = UINT32_MAX;
    uint32_t graphics = UINT32_MAX;

    bool isComplete() const { return present != UINT32_MAX && graphics != UINT32_MAX; }
};

class Context {
public:

    Context() = default;
    ~Context() = default;
    Context(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    struct ExtendFunction {
        PFN_vkQueueSubmit2KHR pVkQueueSubmit2KHR;
        PFN_vkCmdBlitImage2KHR pVkCmdBlitImage2KHR;
        PFN_vkCmdPipelineBarrier2KHR pVkCmdPipelineBarrier2KHR;
        PFN_vkCmdBeginRenderingKHR pVkCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR pVkCmdEndRenderingKHR;
    } extendFunction;

public:

    void Init();
    void Finalize();

    VkInstance GetVkInstance() const { return vkInstance_; }
    VkPhysicalDevice GetVkPhysicalDevice() const { return chosenGPU_; }
    VkQueue GetGraphicQueue() const { return graphicsQueue_; }
    VkQueue GetPresentQueue() const { return presentQueue_; }
    VkDevice GetVkDevice() const { return vkDevice_; }
    VmaAllocator GetVmaAllocator() const { return vmaAllocator_; }
    VkSwapchainKHR GetVkSwapChain() const { return swapChain_;}
    VkExtent2D GetSwapChainImageExtent() const { return swapChainExtent_; }
    VkFormat GetSwapChainImageFormat() const {return swapChainImageFormat_; }
    SwapChainImage GetSwapChainImage(u8 index) const { return swapChainImages_[index]; }
    QueueFamilyIndices GetQueueFamilyIndices() const { return queueFamilyIndices_; }
    GLFWwindow* GetGLFWWindow() const { return window_; }

    void ResizeSwapchain();
    
    // immediate submmit
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
private:

    // init render device
    void CreateVkInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void PickGPU();
    void CreateLogicalDevice();
    void CreateMemoryAllocator();
	void CreateSwapChain();
    void DestroySwapChain();
    void InitExtendFunctions();

    /**************************/
	/**** HELPER FUNCTIONS ****/
	/**************************/
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities = {};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device) const;
    bool checkInstanceExtensionSupport(const std::vector<const char*>& extensions) const;
    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const;
    std::vector<const char*> GetRequiredExtensions() const;
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablemodes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    SwapChainSupportDetails GetSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    // debug callback function，VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
    static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:

    // glfw window
    GLFWwindow* window_;
    VkExtent2D windowExtent_;

    // Vulkan objects
    VkInstance vkInstance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice chosenGPU_;
    QueueFamilyIndices queueFamilyIndices_;
    VkDevice vkDevice_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VmaAllocator vmaAllocator_;
    VkSwapchainKHR swapChain_;
	VkExtent2D swapChainExtent_;
    VkFormat swapChainImageFormat_;
    std::vector<SwapChainImage> swapChainImages_;

    // Built in structures for immediate submmit
    VkFence immFence_;
    VkCommandBuffer immCommandBuffer_;
    VkCommandPool immCommandPool_;

};

}
