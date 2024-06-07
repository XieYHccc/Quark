#include "pch.h"
#include "Rendering/Vulkan/VulkanDevice.h"
#include "Rendering/Vulkan/RenderDevice_Vulkan.h"

//TODO: Make this configurable
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

VulkanDevice::VulkanDevice(RenderDevice_Vulkan& driver)
    :driver_(driver)
{
    CORE_ASSERT(driver.vkInstance != VK_NULL_HANDLE && driver.vkSurface != VK_NULL_HANDLE)

    PickGPU(driver_.vkInstance, driver_.vkSurface);
    CreateLogicalDevice();
    CreateAllocator();
}

VulkanDevice::~VulkanDevice()
{
    CORE_LOGD("Destroying vma allocator...")
    vmaDestroyAllocator(vmaAllocator);

    vkDestroyDevice(logicalDevice, nullptr);
}

void VulkanDevice::PickGPU(VkInstance instance, VkSurfaceKHR surface)
{
    CORE_LOGI("Createing vulkan physical device...")

    // Get all GPUs that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    CORE_ASSERT_MSG(deviceCount !=0, "failed to find GPUs with Vulkan support!")

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsPhysicalDeviceSuitable(device, surface)) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory);

            CORE_LOGI("GPU \"{}\" is selected.", properties.deviceName)

            physicalDevice = device;
            auto indices = GetQueueFamilyIndices(physicalDevice, surface);
            graphicQueueIndex = indices.graphics;
            presentQueueIndex = indices.present;
            transferQueueIndex = indices.transfer;
            computeQueueIndex = indices.compute;

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

            CORE_LOGI("Graphic queue family index: {}", graphicQueueIndex)
            CORE_LOGI("Present queue family index: {}", presentQueueIndex)
            CORE_LOGI("Transfer queue family index: {}", transferQueueIndex)
            CORE_LOGI("Compute queue family index: {}", computeQueueIndex)

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

    CORE_ASSERT_MSG(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU!")

}

void VulkanDevice::CreateLogicalDevice()
{
    CORE_LOGI("Creating vulkan logical device...")

    // required queues for logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    std::set<uint32_t> uniqueQueueFamilies = { graphicQueueIndex, presentQueueIndex,
         transferQueueIndex};

    // get queue family properties
    // uint32_t queueFamilyCount = 0;
    // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    // std::vector<VkQueueFamilyProperties> props(queueFamilyCount);
    // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, props.data());
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

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice))
    CORE_LOGI("Vulkan logical device created")

    // store the queue handle
    vkGetDeviceQueue(logicalDevice, graphicQueueIndex, 0, &graphicQueue);
    vkGetDeviceQueue(logicalDevice, presentQueueIndex, 0, &presentQueue);
    vkGetDeviceQueue(logicalDevice, transferQueueIndex, 0, &transferQueue);

}

void VulkanDevice::CreateAllocator()
{
    VmaAllocatorCreateInfo vmaInfo = {};
    vmaInfo.physicalDevice = physicalDevice;
    vmaInfo.device = logicalDevice;
    vmaInfo.instance = driver_.vkInstance;
    vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VK_CHECK(vmaCreateAllocator(&vmaInfo, &vmaAllocator))
    CORE_LOGI("Vma allocator Created")
}


bool VulkanDevice::IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const
{

    // Get basic properties like name, type and supported vulkan version
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Get features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // TODO: These requirements should be made configurable
    physicalDeviceRequirements requirements;
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_dynamic_rendering",
        "VK_KHR_synchronization2",
        "VK_KHR_copy_commands2"
    };
#ifdef __APPLE__
    deviceExtensions.push_back("VK_KHR_portability_subset");
    requirements.DescreteGpu = false;
#endif

    requirements.deviceExtensions = deviceExtensions;

    CORE_LOGD("Checking GPU \"{}\"...", deviceProperties.deviceName)

    // Descrete GPU？
    if (requirements.DescreteGpu) {
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            CORE_LOGD("  Device is not a discrete GPU, and one is required. Skipping.", deviceProperties.deviceName)
            return false;
        }
    }

    // Anisotropy sampler？
    if (requirements.samplerAnisotropy) {
        if (!deviceFeatures.samplerAnisotropy) {
            CORE_LOGD("  Device do not support anisotropy sampler, which is required. Skipping.", 
                deviceProperties.deviceName)
            return false;
        }
    }

    // Check queue family support
    QueueFamilyIndices indices = GetQueueFamilyIndices(device, surface);
    if (!indices.isComplete())
    {
        CORE_LOGI("  Device do not support required queues. Skipping.")
        return false;
        
    }

    // Check swapchain support
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(device, surface);

    // require at least one format and one present mode
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
    {
        CORE_LOGD("  Device do not support required swapchain supports. Skipping.")
        return false;
    }

    CORE_LOGD("Checking device extesions...")
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Required device extensions:")
    for(const auto& s : requirements.deviceExtensions)
        CORE_LOGD("  {}", s);
#endif

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
            CORE_LOGD("  Required extension not found: {}. Skipping", requirements.deviceExtensions[i]);
            return false;
        }
    }
    
    // Device meets all requirements
    CORE_LOGD("GPU \"{}\" meets all requirements.", deviceProperties.deviceName)
    return true;
}

VulkanDevice::QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    QueueFamilyIndices indices;

    // get queue family properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Look at each queue and see what queues it supports
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queueFamilyCount; ++i) {
        u8 current_transfer_score = 0;

        // Graphics queue?
        if (indices.graphics == UINT32_MAX && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphics = i;
            ++current_transfer_score;

            // If also a present queue, this prioritizes grouping of the 2.
            VkBool32 supports_present = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
            if (supports_present) {
                indices.present = i;
                ++current_transfer_score;
            }
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

        // If a present queue hasn't been found, iterate again and take the first one.
        // This should only happen if there is a queue that supports graphics but NOT
        // present.
        if (indices.present == UINT32_MAX) {
            for (u32 i = 0; i < queueFamilyCount; ++i) {
                VkBool32 supports_present = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
                if (supports_present) {
                    indices.present = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting
                    // purposes.
                    if (indices.present != indices.graphics) {
                        CORE_LOGW("Warning: Different queue index used for present vs graphics: {}.", i);
                    }
                    break;
                }
            }
        }
    }

    return indices;
}

SwapChainSupportDetails VulkanDevice::GetSwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const
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

VkFormat VulkanDevice::GetDepthFormat() const
{
    std::array<VkFormat, 3> candidates= {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (size_t i = 0; i < 3; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            return candidates[i];
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            return candidates[i];
        }
    }

    return VK_FORMAT_UNDEFINED;
}
