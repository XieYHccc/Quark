#pragma once
#include "Rendering/RenderDeviceDriver.h"
#include "Rendering/Vulkan/VulkanTypes.h"
#include "Rendering/Vulkan/VulkanDevice.h"
#include "Rendering/Vulkan/VulkanSwapChain.h"
#include "Rendering/Vulkan/VulkanRenderPass.h"

class RenderDeviceDriver_Vulkan final : public RenderDeviceDriver {
public:
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface;
    VulkanDevice* device;
    VulkanSwapChain* swapChain;
    u32 frameBufferWidth;
    u32 frameBuferHeight;
    RenderPass_Vulkan mainRenderPass;
    std::vector<CommandBuffer_Vulkan> graphicCommandBuffers;
#ifdef QK_DEBUG_BUILD
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

public:
    virtual bool Init() override final;
    virtual void ShutDown() override final;
    virtual bool BeiginFrame(f32 deltaTime) override final;
    virtual bool EndFrame(f32 deltaTime) override final;
    virtual void Resize(u16 width, u16 height) override final;

	/*****************/
	/**** IMAGE ****/
	/*****************/
private:
    struct GPUImage_Vulkan {
        VkImage handle = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        struct {
            VmaAllocation handle = nullptr;
            VmaAllocationInfo info = {};
        } allocation;
    };
public:

    virtual void ImageCreate(const GPUImageDesc& desc, GPUImage* outImage) override final;
    virtual void ImageFree(GPUImage* image) override final;

    /*****************/
	/*** Rendering ***/
	/*****************/
 
    // virtual void RenderPassCreate() override final;
    // virtual void RenderPassFree() override final;
private:
    void CreateVulkanInstance();
    void CreateDebugMessenger();
    void CreateVulkanSurface();
    void CreateCommandBuffers();
    // Get required vulkan instance extensions
    std::vector<const char*> GetRequiredExtensions() const;
    // Debug callback function，VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
    static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

};

