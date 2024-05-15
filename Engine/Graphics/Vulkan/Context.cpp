#include "pch.h"
#include "Graphics/Vulkan/Context.h"

#include <GLFW/glfw3.h>

#include "Core/Window.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Graphics/Vulkan/Initializers.h"

namespace vk {

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_dynamic_rendering",
    "VK_KHR_synchronization2",
    "VK_KHR_copy_commands2"
#ifdef __APPLE__
    ,"VK_KHR_portability_subset"
#endif
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Context::VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << pCallbackData->pMessage << std::endl;

    // abort if it's a error
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) 
        std::abort();

    // if the Vulkan call that triggered the validation layer message should be aborted
    return VK_FALSE;
}


void Context::Init()
{
    // Get application's window
    window_ = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    windowExtent_.width = width;
    windowExtent_.height = height;

    // Create core vulkan objects
    CreateVkInstance();
    CreateSurface();
    PickGPU();
    CreateLogicalDevice();
    CreateMemoryAllocator();
    CreateSwapChain();

    // Create immediate submit structures
    VkCommandPoolCreateInfo commandPoolInfo = vk::init::command_pool_create_info(queueFamilyIndices_.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	vkCreateCommandPool(vkDevice_, &commandPoolInfo, nullptr, &immCommandPool_);
	VkCommandBufferAllocateInfo cmdAllocInfo = vk::init::command_buffer_allocate_info(immCommandPool_, 1);
	vkAllocateCommandBuffers(vkDevice_, &cmdAllocInfo, &immCommandBuffer_);
    VkFenceCreateInfo fenceCreateInfo = vk::init::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    vkCreateFence(vkDevice_, &fenceCreateInfo, nullptr, &immFence_);

    // Init Extendsions
    InitExtendFunctions();
}

void Context::Finalize()
{
    vkDeviceWaitIdle(vkDevice_);

    // destroy immediate submit structures
    vkDestroyFence(vkDevice_, immFence_, nullptr);
    vkDestroyCommandPool(vkDevice_, immCommandPool_, nullptr);

    DestroySwapChain();
    vmaDestroyAllocator(vmaAllocator_);
    vkDestroyDevice(vkDevice_, nullptr);
    vkDestroySurfaceKHR(vkInstance_, surface_, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(vkInstance_, debugMessenger_, nullptr);
    }
    vkDestroyInstance(vkInstance_, nullptr);

    CORE_LOG_INFO("Context Destroyed")

}

void Context::CreateVkInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
        CORE_LOG_ERROR("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "No Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "XEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // add flag for drivers that support portability subset
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // get extensions for window support
    auto extensions = GetRequiredExtensions();
    bool check = checkInstanceExtensionSupport(extensions);
    CORE_ASSERT_MSG(check, "Vkinstance extensions are not supported");

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &vkInstance_));

    CORE_LOG_DEBUG("VkInstance Created")

    // vkdebugmessenger is just a wrapper of debug callback function
    if (enableValidationLayers)
        CreateDebugMessenger();
}

void Context::CreateSurface()
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    VK_ASSERT(glfwCreateWindowSurface(vkInstance_, window_, nullptr, &surface_));
    CORE_LOG_DEBUG("VkSurfaceKHR Created")
}

void Context::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;

    VK_ASSERT(CreateDebugUtilsMessengerEXT(vkInstance_, &createInfo, nullptr, &debugMessenger_));
    CORE_LOG_DEBUG("VkDebugMessenger Created")
}

void Context::PickGPU()
{       
    // get all GPUs that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);

    CORE_ASSERT_MSG(deviceCount !=0, "failed to find GPUs with Vulkan support!")

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isPhysicalDeviceSuitable(device)) {
            chosenGPU_ = device;
            break;
        }
    }

    CORE_ASSERT_MSG(chosenGPU_ != VK_NULL_HANDLE, "failed to find a suitable GPU!")
    CORE_LOG_DEBUG("VkPhysicalDevice Created")
}

void Context::CreateLogicalDevice()
{
    // get queue famliy indices
    queueFamilyIndices_ = GetQueueFamilyIndices(chosenGPU_, surface_);

    // required queues for logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices_.graphics, queueFamilyIndices_.present };
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        // assign priorities(0.0 -1.0) to queues to influence the scheduling of command buffer executio
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // required vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = true;
	features13.synchronization2 = true;
	// required vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;
    // link structures by pNext pointer
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features12;

    // populate logical device create info
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    // enable features by pNext after vulkan 1.1
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = VK_NULL_HANDLE;

    // enable device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // add validation layer information
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
        createInfo.enabledLayerCount = 0;

    VK_ASSERT(vkCreateDevice(chosenGPU_, &createInfo, nullptr, &vkDevice_));
    
    // store the queue handle
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.graphics, 0, &graphicsQueue_);
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.present, 0, &presentQueue_);


    CORE_LOG_DEBUG("VkDevice Created")
}

void Context::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo vmaInfo = {};
    vmaInfo.physicalDevice = chosenGPU_;
    vmaInfo.device = vkDevice_;
    vmaInfo.instance = vkInstance_;
    vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VK_ASSERT(vmaCreateAllocator(&vmaInfo, &vmaAllocator_));
    CORE_LOG_DEBUG("VmaAllocator Created")
}

void Context::CreateSwapChain()
{
    // query swapchain support details
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(chosenGPU_, surface_);

    // choose suitable format, presentmode and resolution
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    // one more image for tripple buffering
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // if maxImageCount = 0, there is no number limit.
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    // populate swapchain create infomation
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.presentMode = presentMode;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT 
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    // how to handle images in different queues
    uint32_t tmpQueueFamilyIndices[] = { queueFamilyIndices_.graphics, queueFamilyIndices_.present };
    if (queueFamilyIndices_.graphics != queueFamilyIndices_.present)
    {
        // share images in graphic and present queue
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = tmpQueueFamilyIndices;
    }
    else
    {   // concurret mode need at least two queues
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // optional
        createInfo.pQueueFamilyIndices = nullptr; // optional
    }

    // no pretransform
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // no need to interacte with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;

    // no need to use old swapchain to create a new swapchain
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create swapchain
    VK_ASSERT(vkCreateSwapchainKHR(vkDevice_, &createInfo, nullptr, &swapChain_));


    // get exact number of created images
    vkGetSwapchainImagesKHR(vkDevice_, swapChain_, &imageCount, nullptr);

    // store swapchain images
    std::vector<VkImage> images;
    images.resize(imageCount);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(vkDevice_, swapChain_, &imageCount, images.data());
    // store format and extent
    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;

    // create swapchain imageviews
    for (int i = 0 ; i < swapChainImages_.size(); i++) {
        swapChainImages_[i].image = images[i];
        VkImageViewCreateInfo view_info = vk::init::imageview_create_info(swapChainImageFormat_, swapChainImages_[i].image, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_ASSERT(vkCreateImageView(vkDevice_, &view_info, nullptr, &swapChainImages_[i].imageView));
    }

    CORE_LOG_DEBUG("VkSwapChain Created")
}

void Context::ResizeSwapchain()
{
	vkDeviceWaitIdle(vkDevice_);
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }
	DestroySwapChain();

	CreateSwapChain();
}

void Context::DestroySwapChain()
{
	for (int i = 0; i < swapChainImages_.size(); i++) {
		vkDestroyImageView(vkDevice_, swapChainImages_[i].imageView, nullptr);
	}

    vkDestroySwapchainKHR(vkDevice_, swapChain_, nullptr);
}

void Context::InitExtendFunctions()
{
    extendFunction.pVkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(
        vkDevice_, "vkQueueSubmit2KHR");
    extendFunction.pVkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(
        vkDevice_, "vkCmdBlitImage2KHR");
    extendFunction.pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(
        vkDevice_, "vkCmdPipelineBarrier2KHR");
    extendFunction.pVkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(
        vkDevice_, "vkCmdBeginRenderingKHR");
    extendFunction.pVkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(
        vkDevice_, "vkCmdEndRenderingKHR");
}

void Context::ImmediateSubmit(std::function<void (VkCommandBuffer)> &&function)
{
    vkResetFences(vkDevice_, 1, &immFence_);
	vkResetCommandBuffer(immCommandBuffer_, 0);

	VkCommandBuffer cmd = immCommandBuffer_;
	VkCommandBufferBeginInfo cmdBeginInfo = vk::init::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(cmd, &cmdBeginInfo);
	function(cmd);
	vkEndCommandBuffer(cmd);

	// submit command buffer to the queue and execute it.
	// immFence_ will now block until the graphic commands finish execution
	VkCommandBufferSubmitInfo cmdinfo = vk::init::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vk::init::submit_info(&cmdinfo, nullptr, nullptr);
	extendFunction.pVkQueueSubmit2KHR(graphicsQueue_, 1, &submit, immFence_);

	vkWaitForFences(vkDevice_, 1, &immFence_, true, 9999999999);
}

/**********************************************************************************/
/******************************* HELPER FUNCTIONS *********************************/
/**********************************************************************************/
std::vector<const char*> Context::GetRequiredExtensions() const
{
    // get extensions for glfw
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions;

    for (unsigned int i = 0; i < glfwExtensionCount; i++)
        extensions.push_back(glfwExtensions[i]);

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;

}

bool Context::isPhysicalDeviceSuitable(VkPhysicalDevice device) const
{
    // get basic properties like name, type and supported vulkan version
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

#ifndef __APPLE__
    // require descrete gpu
    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return false;
#endif

    // check the support for optional features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // require anisotropysample
    if (!deviceFeatures.samplerAnisotropy)
        return false;

    // query quefamily indices
    QueueFamilyIndices indices = GetQueueFamilyIndices(device, surface_);
    if (!indices.isComplete())
        return false;

    // get swapchain support details
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(device, surface_);
    // require at least one format and one present mode
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate)
        return false;

    // check device extension support
    if (!CheckDeviceExtensionSupport(device, deviceExtensions))
        return false;

    return true;
}

VkSurfaceFormatKHR Context::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // look for ideal format
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && (
            availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM || 
            availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM))
            return availableFormat;
    }

    // choose the first format if ideal format not available
    return availableFormats[0];
}

VkPresentModeKHR Context::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
    // 在Vulkan中有四个模式可以使用:
    // 1，VK_PRESENT_MODE_IMMEDIATE_KHR
    // 应用程序提交的图像被立即传输到屏幕呈现，这种模式可能会造成撕裂效果。
    // 2，VK_PRESENT_MODE_FIFO_KHR
    // 交换链被看作一个队列，当显示内容需要刷新的时候，显示设备从队列的前面获取图像，并且程序将渲染完成的图像插入队列的后面。如果队列是满的程序会等待。这种规模与视频游戏的垂直同步很类似。显示设备的刷新时刻被称为“垂直中断”。
    // 3，VK_PRESENT_MODE_FIFO_RELAXED_KHR
    // 该模式与上一个模式略有不同的地方为，如果应用程序存在延迟，即接受最后一个垂直同步信号时队列空了，将不会等待下一个垂直同步信号，而是将图像直接传送。这样做可能导致可见的撕裂效果。
    // 4，VK_PRESENT_MODE_MAILBOX_KHR
    // 这是第二种模式的变种。当交换链队列满的时候，选择新的替换旧的图像，从而替代阻塞应用程序的情形。这种模式通常用来实现三重缓冲区，与标准的垂直同步双缓冲相比，它可以有效避免延迟带来的撕裂效果。

    // default mode
    VkPresentModeKHR defaultMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return defaultMode;
}

VkExtent2D Context::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
{

    // if currentExtent equals std::numeric_limits<uint32_t>::max(), we can customize extent
    // otherwise we can only use given extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

QueueFamilyIndices Context::GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    QueueFamilyIndices indices;

    // get queue family properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // loop to find required queue families
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // require graphic support
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics = i;

        // require surface support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport)
            indices.present = i;

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

Context::SwapChainSupportDetails Context::GetSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    SwapChainSupportDetails details;

    // what swapchain support is according to physical device and surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // query format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // query present mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool Context::checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

bool Context::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    // 如果全部删除完了，说明我们所需要的基本扩展都是支持的
    return requiredExtensions.empty();
}

bool Context::checkInstanceExtensionSupport(const std::vector<const char*>& extensions) const{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // 如果全部删除完了，说明我们所需要的基本扩展都是支持的
    return requiredExtensions.empty();
}


}
