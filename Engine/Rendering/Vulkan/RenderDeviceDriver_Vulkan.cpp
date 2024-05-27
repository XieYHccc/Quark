#include "pch.h"
#include "Rendering/Vulkan/RenderDeviceDriver_Vulkan.h"

#include <GLFW/glfw3.h>

#include "Core/Window.h"


/*****************/
/**** GENERIC ****/
/*****************/
constexpr VkFormat _ConvertFormat(GPUImageFormat value)
{
    switch(value)
    {
    case GPUImageFormat::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case GPUImageFormat::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case GPUImageFormat::D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case GPUImageFormat::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    return VK_FORMAT_UNDEFINED;

}

constexpr VkImageType _ConvertImageType(GPUImageType type)
{
    switch (type) 
    {
    case GPUImageType::TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_2D_ARRAY:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_CUBE_ARRAY:
        return VK_IMAGE_TYPE_2D;
}
}

constexpr VkImageViewType _Convert2ImageViewType(GPUImageType type)
{
    switch (type) 
    {
    case GPUImageType::TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case GPUImageType::TYPE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case GPUImageType::TYPE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case GPUImageType::TYPE_CUBE_ARRAY:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
}

constexpr VkSampleCountFlagBits _ConvertSampleBits(GPUImageSamples sample)
{
    switch (sample) 
    {
    case GPUImageSamples::SAMPLES_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case GPUImageSamples::SAMPLES_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case GPUImageSamples::SAMPLES_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case GPUImageSamples::SAMPLES_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case GPUImageSamples::SAMPLES_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case GPUImageSamples::SAMPLES_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case GPUImageSamples::SAMPLES_64:
        return VK_SAMPLE_COUNT_64_BIT;
    }
}

bool RenderDeviceDriver_Vulkan::Init() 
{
    CORE_LOGI("===================Init Vulkan Driver======================")

    CreateVulkanInstance();
#ifdef QK_DEBUG_BUILD
    CreateDebugMessenger();
#endif
    CreateVulkanSurface();

    this->device = new VulkanDevice(*this);
    this->swapChain = new VulkanSwapChain(*this, frameBufferWidth, frameBuferHeight);

    VulkanRenderPassCreate(this, &mainRenderPass, 
        {0, 0, frameBufferWidth, frameBuferHeight},
        {0.f, 0.f, 0.2f, 1.f},
        1.f,
        0);
    CORE_LOGI("Main Vulkan render pass created")

    CreateCommandBuffers();
    

    CORE_LOGI("================Vulkan Driver Initialized==================")

    return true;
}

void RenderDeviceDriver_Vulkan::ShutDown() 
{
    CORE_LOGI("=================ShutDown Vulkan Driver====================")

    CORE_LOGI("Destroying main vulkan render pass...")
    VulkanRenderPassFree(this, &mainRenderPass);

    CORE_LOGI("Destroying vulkan swapchain...")
    delete swapChain;
    
    CORE_LOGI("Destroying vulkan device...")
    delete device;
    
    CORE_LOGI("Destroying vulkan surface...")
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

#ifdef QK_DEBUG_BUILD
    CORE_LOGI("Destroying debug messenger...")
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    func(vkInstance, debugMessenger, nullptr);
#endif

    CORE_LOGI("Destroying vulkan instance...")
    vkDestroyInstance(vkInstance, nullptr);

    CORE_LOGI("===========================================================")

}

bool RenderDeviceDriver_Vulkan::BeiginFrame(f32 deltaTime) 
{
    return true;
}

bool RenderDeviceDriver_Vulkan::EndFrame(f32 deltaTime) 
{
    return true;
}

void RenderDeviceDriver_Vulkan::Resize(u16 width, u16 height) 
{

}

void RenderDeviceDriver_Vulkan::ImageCreate(const GPUImageDesc &desc, GPUImage *outImage)
{
    CORE_DEBUG_ASSERT(outImage->internal == nullptr)
    
    auto internal_state = new GPUImage_Vulkan();

	VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.format = _ConvertFormat(desc.format);
    create_info.imageType = _ConvertImageType(desc.type);
    create_info.extent.width = desc.width;
    create_info.extent.height = desc.height;
    create_info.extent.depth = desc.depth;
    create_info.mipLevels = desc.mipLevels;
    create_info.arrayLayers = desc.arraySize;
    create_info.samples = _ConvertSampleBits(desc.samples);
    create_info.tiling = (desc.usageBits & GPUIMAGE_USAGE_CPU_READ_BIT)? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

    if (desc.usageBits & GPUIMAGE_USAGE_SAMPLING_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_STORAGE_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_CAN_COPY_FROM_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_CAN_COPY_TO_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    // TODO: Make sharing mode configurable
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Allocate memory

    // TODO: allocate a pool for small allocation
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.flags = (desc.usageBits & GPUIMAGE_USAGE_CPU_READ_BIT)? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    // Create image
    VK_CHECK(vmaCreateImage(device->vmaAllocator, &create_info, &allocCreateInfo, &internal_state->handle, &internal_state->allocation.handle, &internal_state->allocation.info))

    // Create imageview
    VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_create_info.image = internal_state->handle;
    image_view_create_info.viewType = _Convert2ImageViewType(desc.type);
    image_view_create_info.format = _ConvertFormat(desc.format);
    // TODO: Make components configurable
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.levelCount = create_info.mipLevels;
	image_view_create_info.subresourceRange.layerCount = create_info.arrayLayers;
    if (desc.usageBits & GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VK_CHECK(vkCreateImageView(device->logicalDevice, &image_view_create_info, nullptr, &internal_state->view))

    // Bookkeep
    outImage->desc = desc;
    outImage->internal = internal_state;
}

void RenderDeviceDriver_Vulkan::ImageFree(GPUImage* image)
{
    GPUImage_Vulkan* internal_img =  (GPUImage_Vulkan*)image->internal;

    vkDestroyImageView(device->logicalDevice, internal_img->view, nullptr);
    if (internal_img->allocation.handle) {
        vmaDestroyImage(device->vmaAllocator, internal_img->handle, internal_img->allocation.handle);
    }
    image->internal = nullptr;
}

void RenderDeviceDriver_Vulkan::CreateVulkanInstance() {
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
    CORE_LOGI("Validation layers enabled. Checking...")

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // checking
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

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vkInstance))
    CORE_LOGI("Vulkan instance created")

}

void RenderDeviceDriver_Vulkan::CreateDebugMessenger()
{
    CORE_LOGD("Createing vulkan debug messenger...")
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;

#ifdef QK_DEBUG_BUILD
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    CORE_ASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(vkInstance, &createInfo, nullptr, &debugMessenger))
#endif

    CORE_LOGD("Vulkan debug messenger created")
}

void RenderDeviceDriver_Vulkan::CreateVulkanSurface()
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    VK_CHECK(glfwCreateWindowSurface(vkInstance, (GLFWwindow*)Window::Instance()->GetNativeWindow(), nullptr, &vkSurface))
    CORE_LOGI("Vulkan surface created")
}

void RenderDeviceDriver_Vulkan::CreateCommandBuffers()
{
    if (graphicCommandBuffers.empty()) {
        graphicCommandBuffers.resize(swapChain->images.size());
    }

    for (u32 i = 0; i < swapChain->images.size(); ++i) {
        if (graphicCommandBuffers[i].handle) {
            vulkan_command_buffer_free(
                this,
                device->graphicCommandPool,
                &graphicCommandBuffers[i]);
        }
        vulkan_command_buffer_allocate(
            this,
            device->graphicCommandPool,
            true,
            &graphicCommandBuffers[i]);
    }

    CORE_LOGI("Vulkan command buffers created.");
}

VKAPI_ATTR VkBool32 VKAPI_CALL RenderDeviceDriver_Vulkan::VkDebugCallback(
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

std::vector<const char*> RenderDeviceDriver_Vulkan::GetRequiredExtensions() const
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
