#include "pch.h"
#include "Graphic/Vulkan/Context_Vulkan.h"
#include <GLFW/glfw3.h>
#include "Core/Window.h"

struct GLFWwindow;

namespace graphic {

VulkanContext::VulkanContext()
{
    CreateInstance();
#ifdef QK_DEBUG_BUILD
    CreateDebugMessenger();
#endif
    CreateSurface();
    SelectPhysicalDevice();
    CreateLogicalDevice();
    CreateVmaAllocator();
    CreateSwapChain();
    InitExtendFunctions();
}

VulkanContext::~VulkanContext()
{
    CORE_LOGI("Destroying Vulkan Context...")
    CORE_LOGD("Destroying swapchain...")
    DestroySwapChain();
    CORE_LOGD("Destroying vma allocator...")
    vmaDestroyAllocator(vmaAllocator);
    CORE_LOGD("Destroying logical device...")
    vkDestroyDevice(logicalDevice, nullptr);
    CORE_LOGD("Destroying vulkan surface...")
    vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Destroying debug messenger...")
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    func(instance, debugMessenger, nullptr);
#endif

    CORE_LOGD("Destroying vulkan instance...")
    vkDestroyInstance(instance, nullptr);

    CORE_LOGD("Vulkan Context destroyed")
}

void VulkanContext::CreateInstance()
{
    CORE_LOGI("Creating vulkan instance...")
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Quark Engine Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Quark Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Get supported extensions and layers
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Get required extensions and layers
    const std::vector<const char*> required_extensions = GetRequiredExtensions();
    std::vector<const char*> required_layers;

    // TODO:Try other layers if this is not available
    required_layers.push_back("VK_LAYER_KHRONOS_validation");

#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Required vulkan instance extensions:")
    for(const auto& s : required_extensions)
        CORE_LOGD("  {}", s);
    CORE_LOGD("Required vulkan instance layers:")
    for(const auto& s : required_layers)
        CORE_LOGD("  {}", s);
#endif

    // Check extensions and layers support infomation
    CORE_LOGD("Checking vulkan instance extensions support...")
    for (u32 i = 0; i < required_extensions.size(); ++i) {
        bool found = false;
        for (u32 j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(required_extensions[i], availableExtensions[j].extensionName)==0) {
                found = true;
                break;
            }
        }

        if (!found) {
            CORE_LOGC("  Required extension not found: {}", required_extensions[i]);
        }
    }
    CORE_LOGD("All required vulkan instance extensions are supported.")

    // Enable validation layers?
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Validation layers enabled. Checking...")

    // TODO: if this is not supported, try other layers
    required_layers.push_back("VK_LAYER_KHRONOS_validation");

    // checking
    for (auto layerName : required_layers) {
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

    CORE_LOGD("All required vulkan validation layers are supported.")
#endif

    // Finally, create instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    createInfo.ppEnabledExtensionNames = required_extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
    createInfo.ppEnabledLayerNames = required_layers.data();
    // Add flag for drivers that support portability subset
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance))

    CORE_LOGI("Vulkan instance created") 
}

void VulkanContext::CreateDebugMessenger()
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
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    CORE_ASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(instance, &createInfo, nullptr, &debugMessenger))
    CORE_LOGD("Vulkan debug messenger created")
#endif
}

void VulkanContext::CreateSurface()
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    VK_CHECK(glfwCreateWindowSurface(instance, (GLFWwindow*)Window::Instance()->GetNativeWindow(), nullptr, &surface))
    CORE_LOGI("Vulkan surface created") 
}

void VulkanContext::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<uint32_t> unique_queue_families = { graphicQueueIndex, computeQueueIndex, transferQueueIndex };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
        uniqueQueueFamilies.push_back(queueFamily);
    }

// TODO: Remove this in the future
#ifdef QK_PLATFORM_MACOS
    // Moltenvk hasn't support vulkan 1.3. Enable 1.3 features manualy
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
#endif

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.pNext = &features2;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice));

    CORE_LOGI("Logical device created")

    // store the queue handle
    vkGetDeviceQueue(logicalDevice, graphicQueueIndex, 0, &graphicQueue);
    vkGetDeviceQueue(logicalDevice, transferQueueIndex, 0, &transferQueue);
    vkGetDeviceQueue(logicalDevice, computeQueueIndex, 0, &computeQueue);

}

void VulkanContext::SelectPhysicalDevice()
{
    CORE_LOGI("Selecting vulkan physical device...")

    // Get all GPUs that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    CORE_ASSERT_MSG(deviceCount !=0, "failed to find GPUs with Vulkan support!")

    // TODO: Make this configurable. For now, just hard code requirements
    std::vector<const char*> required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_dynamic_rendering",
        "VK_KHR_synchronization2",
        "VK_KHR_copy_commands2"
    };
#ifdef __APPLE__
        required_extensions.push_back("VK_KHR_portability_subset");
#endif

    // TODO: Make this configurable
    physicalDeviceRequirement requirements = {};
    requirements.deviceExtensions = required_extensions;
    requirements.ForceDescreteGpu = false;
    requirements.preferDescreteGPU = true;
    // Check gpus
    for (const auto& device : devices) {
        if (IsPhysicalDeviceSuitable(device, requirements)) {
            if (requirements.preferDescreteGPU) {
                if (properties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    physicalDevice = device;
                    break;
                }
            }
            
            if (physicalDevice == VK_NULL_HANDLE) {
                physicalDevice = device;
            }
        }
    }
    CORE_ASSERT_MSG(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU")
    CORE_LOGI("GPU \"{}\" is selected.", properties2.properties.deviceName)

    // Find queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Query base queue families:
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        auto& queue_family_props = queueFamilies[i];
        if (graphicQueueIndex == VK_QUEUE_FAMILY_IGNORED && queue_family_props.queueCount > 0 && queue_family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            graphicQueueIndex = i;
        if (transferQueueIndex == VK_QUEUE_FAMILY_IGNORED && queue_family_props.queueCount > 0 && queue_family_props.queueFlags & VK_QUEUE_TRANSFER_BIT)
            transferQueueIndex = i;
        if (computeQueueIndex == VK_QUEUE_FAMILY_IGNORED && queue_family_props.queueCount > 0 && queue_family_props.queueFlags & VK_QUEUE_COMPUTE_BIT)
            computeQueueIndex = i;
    }

    // Now try to find dedicated compute and transfer queues:
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        auto& queue_family_props = queueFamilies[i];
        if (queue_family_props.queueCount > 0 &&
            queue_family_props.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(queue_family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queue_family_props.queueFlags & VK_QUEUE_COMPUTE_BIT)
            )
        {
            transferQueueIndex = i;
        }

        if (queue_family_props.queueCount > 0 &&
            queue_family_props.queueFlags & VK_QUEUE_COMPUTE_BIT &&
            !(queue_family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            )
        {
            computeQueueIndex = i;
        }
    }

    // Now try to find dedicated transfer queue with only transfer and sparse flags:
    //	(This is a workaround for a driver bug with sparse updating from transfer queue)
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        auto& queue_family_pros = queueFamilies[i];

        if (queue_family_pros.queueCount > 0 && queue_family_pros.queueFlags == (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT))
        {
            transferQueueIndex = i;
        }
    }

    CORE_LOGI("------------GPU Information------------")
    CORE_LOGI("GPU name: {}", properties2.properties.deviceName)
    switch (properties2.properties.deviceType) {
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

    CORE_LOGI("Graphic queue family index: {}", graphicQueueIndex)
    CORE_LOGI("Transfer queue family index: {}", transferQueueIndex)
    CORE_LOGI("Compute queue family index: {}", computeQueueIndex)

    CORE_LOGI(
        "GPU Driver version: {}.{}.{}",
        VK_VERSION_MAJOR(properties2.properties.driverVersion),
        VK_VERSION_MINOR(properties2.properties.driverVersion),
        VK_VERSION_PATCH(properties2.properties.driverVersion));

    // Vulkan API version.
    CORE_LOGI(
        "Vulkan API version: {}.{}.{}",
        VK_VERSION_MAJOR(properties2.properties.apiVersion),
        VK_VERSION_MINOR(properties2.properties.apiVersion),
        VK_VERSION_PATCH(properties2.properties.apiVersion));

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryPorps);
    // Memory information
    for (u32 j = 0; j < memoryPorps.memoryHeapCount; ++j) {
        f32 memory_size_gib = (((f32)memoryPorps.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
        if (memoryPorps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            CORE_LOGI("Local GPU memory: {} GiB", memory_size_gib);
        } else {
            CORE_LOGI("Shared System memory: {} GiB", memory_size_gib);
        }
    }
    CORE_LOGI("---------------------------------------")
}

void VulkanContext::CreateSwapChain()
{
    u32 window_width = Window::Instance()->GetFrambufferWidth();
    u32 window_height = Window::Instance()->GetFrambufferWidth();

    SwapChainSupportDetail swapchain_support = GetSwapchainSupportDetails();

    // Choose a swap surface format.
    bool found = false;
    for (auto& f : swapchain_support.formats) {
        // Preferred formats
        if ((f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM ) && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            found = true;
            break;
        }
    }
    if (!found) {
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    
    // Set present mode
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto& mode : swapchain_support.presentModes) {
        // Preferred mode
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    // Set swapchain extent
    if (swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {
        // Must use gpu specified extent
        swapChainExtent = swapchain_support.capabilities.currentExtent;
    }
    else {
        CORE_LOGW("Customized width and height are used to create swapchain")
        // Clamp to the value allowed by the GPU.
        VkExtent2D min = swapchain_support.capabilities.minImageExtent;
        VkExtent2D max = swapchain_support.capabilities.maxImageExtent;
        swapChainExtent.width = std::clamp(swapChainExtent.width, min.width, max.width);
        swapChainExtent.height = std::clamp(swapChainExtent.height, min.height, max.height);
    }

    u32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    // Swapchain create info
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surfaceFormat.format;
    swapchain_create_info.imageColorSpace = surfaceFormat.colorSpace;
    swapchain_create_info.imageExtent = swapChainExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = swapchain_support.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = presentMode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &swapchain_create_info, nullptr, &swapChain));

    // Get swapchain images
    u32 imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, 0));
    swapChianImages.resize(imageCount);
    swapChainImageViews.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapChain, &image_count, swapChianImages.data()));

    // Create swapchain render color attachments
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapChianImages[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = surfaceFormat.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(logicalDevice, &view_info, nullptr, &swapChainImageViews[i]));
    }

    CORE_LOGI("Swapchain created. Width: {}, Height: {}, image count: {}.", swapChainExtent.width, swapChainExtent.height, swapChianImages.size())

}

void VulkanContext::DestroySwapChain()
{
    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        // Destroy image views
        for (auto& view : swapChainImageViews) {
            vkDestroyImageView(logicalDevice, view, nullptr);
        }
    }
}

void VulkanContext::CreateVmaAllocator()
{
    VmaAllocatorCreateInfo vmaInfo = {};
    vmaInfo.physicalDevice = physicalDevice;
    vmaInfo.device = logicalDevice;
    vmaInfo.instance = instance;
    vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VK_CHECK(vmaCreateAllocator(&vmaInfo, &vmaAllocator));
    CORE_LOGD("VmaAllocator Created")
}

SwapChainSupportDetail VulkanContext::GetSwapchainSupportDetails() const 
{
    SwapChainSupportDetail details;

    // what swapchain support is according to physical device and surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    // query format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    // query present mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanContext::IsPhysicalDeviceSuitable(VkPhysicalDevice device, physicalDeviceRequirement& requirements)
{
    // This function checks the requirements that must be meeted by GPU

    // Fill properties and features
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features2.pNext = &features11;
    features11.pNext = &features12;
    features12.pNext = &features13;

    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    properties13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    properties2.pNext = &properties11;
    properties11.pNext = &properties12;
    properties12.pNext = &properties13;
    vkGetPhysicalDeviceProperties2(device, &properties2);
    vkGetPhysicalDeviceFeatures2(device, &features2);

    CORE_LOGD("Checking GPU \"{}\"...", properties2.properties.deviceName)

    // Query supported device extensions
    uint32_t extensionCount = 0;
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
            CORE_LOGD("  Required extension not found: {}. Skipping", requirements.deviceExtensions[i]);
            return false;
        }
    }

    // TODO: May includes other non-force extensions
    enabledExtensions = requirements.deviceExtensions;

    // Descrete GPU？
    if (requirements.ForceDescreteGpu) {
        if (properties2.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            CORE_LOGD("  Device is not a discrete GPU, and one is required. Skipping.")
            return false;
        }
    }
    // Anisotropy sampler？
    if (requirements.samplerAnisotropy) {
        if (!features2.features.samplerAnisotropy) {
            CORE_LOGD("  Device do not support anisotropy sampler. Skipping.")
            return false;
        }
    }

#ifndef QK_PLATFORM_MACOS
    // Require dynamic rendering except on Moltenvk
    if (!features13.dynamicRendering) {
        CORE_LOGD("  Device do not support dynamic rendering. Skipping.")
        return false;
    }
    // Require synchonazition2 except on Moltenvk
    if (!features13.dynamicRendering) {
        CORE_LOGD("  Device do not support synchronization2. Skipping.")
        return false;
    }
#endif

    // Require device buffer address
    if (!features12.bufferDeviceAddress) {
        CORE_LOGD("  Device do not support buffer device address. Skipping.")
        return false;
    }
    // Require descriptor indexing
    if (!features12.descriptorIndexing) {
        CORE_LOGD("  Device do not support descriptor indexing. Skipping.")
        return false;
    }
    // This GPU is suitable
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::VkDebugCallback(
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

std::vector<const char*> VulkanContext::GetRequiredExtensions() const
{
    //TODO: make this as an interface of Window class
    // Get extensions for glfw
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

void VulkanContext::InitExtendFunctions()
{
    extendFunction.pVkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(
        logicalDevice, "vkQueueSubmit2KHR");
    extendFunction.pVkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(
        logicalDevice, "vkCmdBlitImage2KHR");
    extendFunction.pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(
        logicalDevice, "vkCmdPipelineBarrier2KHR");
    extendFunction.pVkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(
        logicalDevice, "vkCmdBeginRenderingKHR");
    extendFunction.pVkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(
        logicalDevice, "vkCmdEndRenderingKHR");

    CORE_LOGD("Vulkan Extend functions Found")
}
}