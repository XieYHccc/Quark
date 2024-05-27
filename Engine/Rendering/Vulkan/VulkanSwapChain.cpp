#include "pch.h"
#include "Rendering/Vulkan/VulkanSwapChain.h"
#include "Rendering/Vulkan/RenderDeviceDriver_Vulkan.h"

VulkanSwapChain::VulkanSwapChain(RenderDeviceDriver_Vulkan& driver, u32 width, u32 height)
    : driver_(driver)
{
    // Simnply create a new one
    CreateSwapChain(width, height);

}

VulkanSwapChain::~VulkanSwapChain()
{
    DestroySwapChain();
}

void VulkanSwapChain::Resize(u32 width, u32 height)
{
    CORE_LOGI("Recreating swapchain...")

    // Delete the old one and create a new one
    DestroySwapChain();
    CreateSwapChain(width, height);
}

bool VulkanSwapChain::AquireNextImageIndex(VkSemaphore imageAvailableSemaphore, VkFence fence, u64 timeoutNs, u32 outImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(
        driver_.device->logicalDevice,
        handle,
        timeoutNs,
        imageAvailableSemaphore,
        fence,
        &outImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Trigger swapchain recreation, then boot out of the render loop.
        Resize(driver_.frameBufferWidth, driver_.frameBuferHeight);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        CORE_LOGC("Failed to acquire swapchain image!");
        return false;
    }

    return true;
}

void VulkanSwapChain::Present(VkQueue graphicQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 imgeIndex)
{
    // Return the image to the swapchain for presentation.
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &renderCompleteSemaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &handle;
    present_info.pImageIndices = &imgeIndex;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(presentQueue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
        Resize(driver_.frameBufferWidth, driver_.frameBuferHeight);
    } else if (result != VK_SUCCESS) {
        CORE_LOGC("Failed to present swap chain image!");
    }
}

void VulkanSwapChain::CreateSwapChain(u32 width, u32 height)
{
    swapChainExtent = {width, height};
    maxFramesInFlight = 2;

    // Choose a swap surface format.
    SwapChainSupportDetails details = driver_.device->GetSwapChainSupportDetails(driver_.device->physicalDevice, driver_.vkSurface);
    bool found = false;
    for (auto& f : details.formats) {
        // Preferred formats
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            found = true;
            break;
        }
    }
    if (!found) {
        surfaceFormat = details.formats[0];
    }
    // Set present mode
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto& mode : details.presentModes) {
        // Preferred mode
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    // Set swapchain extent
    if (details.capabilities.currentExtent.width != UINT32_MAX) {
        swapChainExtent = details.capabilities.currentExtent;
    }

    // Clamp to the value allowed by the GPU.
    VkExtent2D min = details.capabilities.minImageExtent;
    VkExtent2D max = details.capabilities.maxImageExtent;
    swapChainExtent.width = std::clamp(swapChainExtent.width, min.width, max.width);
    swapChainExtent.height = std::clamp(swapChainExtent.height, min.height, max.height);

    u32 image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }

    // Swapchain create info
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = driver_.vkSurface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surfaceFormat.format;
    swapchain_create_info.imageColorSpace = surfaceFormat.colorSpace;
    swapchain_create_info.imageExtent = swapChainExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices
    if (driver_.device->graphicQueueIndex != driver_.device->presentQueueIndex) {
        u32 queueFamilyIndices[] = {
            driver_.device->graphicQueueIndex,
            driver_.device->graphicQueueIndex};
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }
    swapchain_create_info.preTransform = details.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = presentMode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(driver_.device->logicalDevice, &swapchain_create_info, nullptr, &handle));

    // Start with a zero frame index
    driver_.currentFrame = 0;

    // Get swapchain images and create image views
    u32 imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(driver_.device->logicalDevice, handle, &imageCount, 0));
    images.resize(imageCount);
    imageViews.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(driver_.device->logicalDevice, handle, &image_count, images.data()));

    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = surfaceFormat.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(driver_.device->logicalDevice, &view_info, nullptr, &imageViews[i]));
    }

    // Create depth image and its view
    VkFormat vk_format = driver_.device->GetDepthFormat();
    if (vk_format != VK_FORMAT_UNDEFINED) {
        GPUImageFormat img_format;
        if (vk_format == VK_FORMAT_D32_SFLOAT) 
            img_format = GPUImageFormat::D32_SFLOAT;
        else if (vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            img_format = GPUImageFormat::D32_SFLOAT_S8_UINT;
        else
            img_format = GPUImageFormat::D24_UNORM_S8_UINT;

        GPUImageDesc desc = {
            .arraySize =1,
            .depth = 1,
            .format = img_format,
            .height = swapChainExtent.height,
            .width = swapChainExtent.width,
            .mipLevels = 1,
            .samples = GPUImageSamples::SAMPLES_1,
            .type = GPUImageType::TYPE_2D,
            .usageBits = GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT

        };
        driver_.ImageCreate(desc, &depthimage);

        CORE_LOGI("Vulkan swapchain created. Width: {}, Height: {}, image count: {}.", swapChainExtent.width, swapChainExtent.height, images.size())
    }
    else {
        CORE_LOGC("Failed to find a required depth image format.")
    }
}

void VulkanSwapChain::DestroySwapChain()
{
    // Destroy depth image
    driver_.ImageFree(&depthimage);
    
    // Destroy image views
    for (auto& view : imageViews) {
        vkDestroyImageView(driver_.device->logicalDevice, view, nullptr);
    }
    vkDestroySwapchainKHR(driver_.device->logicalDevice, handle, nullptr);

}