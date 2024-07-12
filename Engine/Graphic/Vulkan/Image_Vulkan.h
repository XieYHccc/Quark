#pragma once
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/Image.h"

namespace graphic {

constexpr VkImageLayout ConvertImageLayout(ImageLayout layout)
{
    switch (layout) {
    case ImageLayout::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::TRANSFER_SRC_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::TRANSFER_DST_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
}

constexpr VkImageType ConvertImageType(ImageType type)
{
    switch (type) 
    {
    case ImageType::TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case ImageType::TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case ImageType::TYPE_3D:
        return VK_IMAGE_TYPE_3D;
    }
}

// This class fills up texture(sampled image)'s mipmap layout and copy informations
// Designed for texture data uploading
class TextureFormatLayout {
	struct MipInfo {
		size_t offset = 0;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;

		uint32_t num_block_x = 0;
		uint32_t num_block_y = 0;
		uint32_t total_block_size_x = 0; //  num_block_x * block_dim_x
		uint32_t total_block_size_y = 0;
	};

    TextureFormatLayout() = default;
    void SetUp1D();
    void SetUp2D(VkFormat format, uint32_t width, uint32_t height, uint32_t array_layers, uint32_t mip_levels);

private:
    VkFormat format_;
    VkImageType image_type_;
    size_t required_size = 0; // required data source size
    uint32_t block_stride_ = 1;
	uint32_t mip_levels_ = 1;
	uint32_t array_layers_ = 1;
	uint32_t block_dim_x_ = 1;
	uint32_t block_dim_y_ = 1;
    MipInfo mips_[16];
};

class Image_Vulkan : public Image {
    friend class Device_Vulkan;
    friend class CommandList_Vulkan;
public:
    Image_Vulkan(const ImageDesc& desc); // only used for fill swapchain image infomation
    Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data);
    virtual ~Image_Vulkan();
    
private:
    
    Device_Vulkan* device_;
    VkImage handle_;
    VmaAllocation allocation_;
    VkImageView view_;
    bool isSwapChainImage_;
};

CONVERT_TO_VULKAN_INTERNAL(Image)

class Sampler_Vulkan : public Sampler {
    friend class Device_Vulkan;
    friend class CommandList_Vulkan;
public:
    Sampler_Vulkan(Device_Vulkan* device, const SamplerDesc& desc);
    ~Sampler_Vulkan();
private:
    Device_Vulkan* device_;
    VkSampler handle_;
};

CONVERT_TO_VULKAN_INTERNAL(Sampler)
}