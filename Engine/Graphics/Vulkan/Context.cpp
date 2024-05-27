#include "pch.h"
#include "Graphics/Vulkan/Context.h"

#include <GLFW/glfw3.h>

#include "Core/Window.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Graphics/Vulkan/Initializers.h"

namespace vk {

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

struct physicalDeviceRequirements
{
    bool graphics { true };
    bool present { true };
    bool compute { true };
    bool transfer { true };
    std::vector<const char*> deviceExtensions;
    bool samplerAnisotropy {true};
    bool DescreteGpu {true};

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

VKAPI_ATTR VkBool32 VKAPI_CALL Context::VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
    void* pUserData)
{
    switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            CORE_LOGE(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            CORE_LOGW(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            CORE_LOGI(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            CORE_LOGT(pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}


void Context::Init()
{
    CORE_LOGI("===================Init Vulkan Context=====================")
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

    CORE_LOGI("===========================================================")
}

void Context::Finalize()
{
    CORE_LOGI("Destroying vulkan context...")

    vkDeviceWaitIdle(vkDevice_);

    // destroy immediate submit structures
    vkDestroyFence(vkDevice_, immFence_, nullptr);
    vkDestroyCommandPool(vkDevice_, immCommandPool_, nullptr);

    DestroySwapChain();
    vmaDestroyAllocator(vmaAllocator_);
    vkDestroyDevice(vkDevice_, nullptr);
    vkDestroySurfaceKHR(vkInstance_, surface_, nullptr);

#ifdef QK_DEBUG_BUILD
    DestroyDebugUtilsMessengerEXT(vkInstance_, debugMessenger_, nullptr);
#endif

    vkDestroyInstance(vkInstance_, nullptr);

    CORE_LOGI("Vulkan Context Destroyed")

}

void Context::CreateVkInstance()
{
    CORE_LOGI("Creating vulkan instance...")
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
    
    // Add flag for drivers that support portability subset
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Get extensions for window support
    auto extensions = GetRequiredExtensions();

    // For debug purpuse
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Required vulkan instance extensions:")
    for(const auto& s : extensions)
        CORE_LOGD("  {}", s);
#endif

    CORE_LOGI("Checking vulkan instance extensions support...")
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for (u32 i = 0; i < extensions.size(); ++i) {
        bool found = false;
        for (u32 j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(extensions[i], availableExtensions[j].extensionName)==0) {
                found = true;
                break;
            }
        }

        if (!found) {
            CORE_LOGC("  Required extension not found: {}", extensions[i]);
        }
    }
    
    // All extensions are supported
    CORE_LOGI("All required vulkan instance extensions are supported.")
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers?
#ifdef QK_DEBUG_BUILD
    CORE_LOGI("Checking vulkan validation layer support...")

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

        if (!layerFound) {
            CORE_LOGC("  Required extension not found: {}", layerName);
        }
            
    }

    CORE_LOGI("All required vulkan validation layers are supported.")
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &vkInstance_));
    CORE_LOGI("Vulkan instance created")

#ifdef QK_DEBUG_BUILD
    CreateDebugMessenger();
#endif

}

void Context::CreateSurface()
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    VK_ASSERT(glfwCreateWindowSurface(vkInstance_, window_, nullptr, &surface_));
    CORE_LOGI("Vulkan Surface Created")
}

void Context::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;

#ifdef QK_DEBUG_BUILD
    VK_ASSERT(CreateDebugUtilsMessengerEXT(vkInstance_, &createInfo, nullptr, &debugMessenger_));
#endif

    CORE_LOGD("Vulkan DebugMessenger Created")
}

void Context::PickGPU()
{   
    CORE_LOGI("Selecting GPU...")
    // Get all GPUs that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);

    CORE_ASSERT_MSG(deviceCount !=0, "failed to find GPUs with Vulkan support!")

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsPhysicalDeviceSuitable(device)) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory);

            CORE_LOGI("GPU {} is selected.", properties.deviceName)

            chosenGPU_ = device;
            queueFamilyIndices_ = GetQueueFamilyIndices(chosenGPU_, surface_);
            CORE_LOGI("------------GPU Information------------")
            CORE_LOGI("GPU name: {}", properties.deviceName)
            
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    CORE_LOGI("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    CORE_LOGI("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    CORE_LOGI("GPU type is Descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    CORE_LOGI("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    CORE_LOGI("GPU type is CPU.");
                    break;
            }

            CORE_LOGI(
                "GPU Driver version: {}.{}.{}",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // Vulkan API version.
            CORE_LOGI(
                "Vulkan API version: {}.{}.{}",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            // Memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    CORE_LOGI("Local GPU memory: {} GiB", memory_size_gib);
                } else {
                    CORE_LOGI("Shared System memory: {} GiB", memory_size_gib);
                }
            }
            CORE_LOGI("---------------------------------------")
            break;
        }
    }

    CORE_ASSERT_MSG(chosenGPU_ != VK_NULL_HANDLE, "failed to find a suitable GPU!")

}

void Context::CreateLogicalDevice()
{
    CORE_LOGI("Creating vulkan logical device...")

    // required queues for logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices_.graphics, queueFamilyIndices_.present,
        queueFamilyIndices_.transfer};
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

    // Required vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = true;
	features13.synchronization2 = true;
	// Required vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;
    // link structures by pNext pointer
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features12;

    // Populate logical device create info
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    // Enable features by pNext after vulkan 1.1
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = VK_NULL_HANDLE;

    // Enable Extensions
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Deprecated
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

    VK_ASSERT(vkCreateDevice(chosenGPU_, &createInfo, nullptr, &vkDevice_));
    CORE_LOGI("Vulkan logical device created")

    // store the queue handle
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.graphics, 0, &graphicsQueue_);
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.present, 0, &presentQueue_);
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.transfer, 0, &transferQueue_);

}

void Context::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo vmaInfo = {};
    vmaInfo.physicalDevice = chosenGPU_;
    vmaInfo.device = vkDevice_;
    vmaInfo.instance = vkInstance_;
    vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VK_ASSERT(vmaCreateAllocator(&vmaInfo, &vmaAllocator_));
    CORE_LOGD("VmaAllocator Created")
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

    CORE_LOGD("VkSwapChain Created")
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

#ifdef QK_DEBUG_BUILD
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return extensions;

}

bool Context::IsPhysicalDeviceSuitable(VkPhysicalDevice device) const
{
    // Get basic properties like name, type and supported vulkan version
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Get features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // TODO: These requirements should be made configurable
    physicalDeviceRequirements requirements;
    requirements.deviceExtensions = deviceExtensions;
#ifdef __APPLE__
    requirements.DescreteGpu = false;
#endif

    CORE_LOGI("Checking GPU {} ...", deviceProperties.deviceName)

    // Descrete GPU？
    if (requirements.DescreteGpu) {
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            CORE_LOGI("  Device is not a discrete GPU, and one is required. Skipping.", deviceProperties.deviceName)
            return false;
        }
    }

    // Anisotropy sampler？
    if (requirements.samplerAnisotropy) {
        if (!deviceFeatures.samplerAnisotropy) {
            CORE_LOGI("  Device do not support anisotropy sampler, which is required. Skipping.", 
                deviceProperties.deviceName)
            return false;
        }
    }

    // Check queue family support
    QueueFamilyIndices indices = GetQueueFamilyIndices(device, surface_);
    if (!indices.isComplete())
    {
        CORE_LOGI("  Device do not support required queues. Skipping.")
        return false;
        
    }

    // Check swapchain support
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(device, surface_);

    // require at least one format and one present mode
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
    {
        CORE_LOGI("  Device do not support required swapchain supports. Skipping.")
        return false;
    }

    // Check device extension support
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (u32 i = 0; i < requirements.deviceExtensions.size(); ++i) {
        bool found = false;
        for (u32 j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(requirements.deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            CORE_LOGI("  Required extension not found: {}. Skipping", requirements.deviceExtensions[i]);
            return false;
        }
    }
    
    // Device meets all requirements
    CORE_LOGI("GPU {} meets all requirements.", deviceProperties.deviceName)
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
    // uint32_t i = 0;
    // for (const auto& queueFamily : queueFamilies)
    // {
    //     // require graphic support
    //     if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    //         indices.graphics = i;

    //     // require surface support
    //     VkBool32 presentSupport = false;
    //     vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    //     if (queueFamily.queueCount > 0 && presentSupport)
    //         indices.present = i;

    //     if (indices.isComplete())
    //         break;

    //     i++;
    // }

    // Look at each queue and see what queues it supports
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queueFamilyCount; ++i) {
        u8 current_transfer_score = 0;

        // Graphics queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
            ++current_transfer_score;
        }

        // Compute queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.compute = i;
            ++current_transfer_score;
        }

        // Transfer queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // Take the index if it is the current lowest. This increases the
            // liklihood that it is a dedicated transfer queue.
            if (current_transfer_score <= min_transfer_score) {
                min_transfer_score = current_transfer_score;
                indices.transfer = i;
            }
        }

        // Present queue?
        VkBool32 supports_present = VK_FALSE;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &supports_present));
        if (supports_present) {
            indices.present = i;
        }
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

// bool Context::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const
// {
//     uint32_t layerCount;
//     vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

//     std::vector<VkLayerProperties> availableLayers(layerCount);
//     vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

//     for (auto layerName : validationLayers) {
//         bool layerFound = false;

//         for (const auto& layerProperties : availableLayers) {
//             if (strcmp(layerName, layerProperties.layerName) == 0) {
//                 layerFound = true;
//                 break;
//             }
//         }

//         if (!layerFound)
//             return false;
//     }

//     return true;
// }

// bool Context::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions) const
// {
//     uint32_t extensionCount;
//     vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

//     std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//     vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

//     std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

//     for (const auto& extension : availableExtensions) {
//         requiredExtensions.erase(extension.extensionName);
//     }

//     // Device support all extensions
//     return true;
// }

// bool Context::CheckInstanceExtensionSupport(const std::vector<const char*>& extensions) const{

//     CORE_LOGI("Checking vulkan instance extensions support.")
//     uint32_t extensionCount = 0;
//     vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//     std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//     vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

//     // std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
//     // for (const auto& extension : availableExtensions) {
//     //     requiredExtensions.erase(extension.extensionName);
//     // }
//     for (u32 i = 0; i < extensions.size(); ++i) {
//         bool found = false;
//         for (u32 j = 0; j < availableExtensions.size(); ++j) {
//             if (strcmp(extensions[i], availableExtensions[j].extensionName)) {
//                 found = true;
//                 break;
//             }
//         }

//         if (!found) {
//             CORE_LOGI("  Required extension not found: {}. Skipping", extensions[i]);
//             return false;
//         }
//     }
    
//     // All extensions are supported
//     CORE_LOGI("All vulkan instance extensions are supported.")
//     return true;
// }


}
