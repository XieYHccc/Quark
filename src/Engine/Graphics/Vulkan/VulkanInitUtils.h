#pragma once
#include "pch.h"
#include <deque>

#include "Graphics/Vulkan/VulkanTypes.h"
#include "Graphics/Vulkan/DescriptorVulkan.h"
namespace vkutil {

struct QueueFamilyIndices
{
    uint32_t present = UINT32_MAX;
    uint32_t graphics = UINT32_MAX;

    bool isComplete() const { return present != UINT32_MAX && graphics != UINT32_MAX; }
};

struct SwapChainSupportDetails
{
    // 基本的surface功能属性(min/max number of images in swap chain, min/max width and height of images)
    VkSurfaceCapabilitiesKHR capabilities = {};
    // Surface格式(pixel format, color space)
    std::vector<VkSurfaceFormatKHR> formats;
    // 有效的presentation模式
    std::vector<VkPresentModeKHR> presentModes;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_deletor(std::function<void()>&& function) 
	{
		deletors.push_back(function);
	}

	void flush() 
	{
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
			(*it)(); //call functors
		deletors.clear();
	}
};

bool checkInstanceExtensionSupport(const std::vector<const char*>& extensions);

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions);

QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

SwapChainSupportDetails GetSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface);

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

}