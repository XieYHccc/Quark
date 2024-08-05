#include "qkpch.h"
#include "Graphic/TextureFormatLayout.h"

namespace graphic {

void TextureFormatLayout::SetUp2D(DataFormat format, uint32_t width, uint32_t height, uint32_t array_size, uint32_t mip_levels)
{
    format_ = format;
    image_type_ = ImageType::TYPE_2D;
    array_size_ = array_size;
    mip_levels_ = mip_levels;
    
    FillMipInfos(width, height, 1);
    
}

uint32_t TextureFormatLayout::GeneratedMipCount(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t size = unsigned(std::max(std::max(width, height), depth));
    uint32_t levels = 0;
    while (size) {
        levels++;
        size >>= 1;
    }
    return levels;
}

void TextureFormatLayout::FillMipInfos(uint32_t width, uint32_t height, uint32_t depth)
{
	block_stride_ = GetFormatStride(format_);
	GetFormatBlockDim(format_, block_dim_x_, block_dim_y_);

    // generate mipmaps
	if (mip_levels_ == 0)
        mip_levels_ = GeneratedMipCount(width, height, depth);

	size_t offset = 0;
	for (uint32_t mip = 0; mip < mip_levels_; mip++)
	{
		offset = (offset + 15) & ~15;

		uint32_t blocks_x = (width + block_dim_x_ - 1) / block_dim_x_;
		uint32_t blocks_y = (height + block_dim_y_ - 1) / block_dim_y_;

		mips_[mip].offset = offset;
		mips_[mip].num_block_x = blocks_x;
		mips_[mip].num_block_y = blocks_y;
		mips_[mip].row_length = blocks_x * block_dim_x_;
		mips_[mip].image_height = blocks_y * block_dim_y_;
		mips_[mip].width = width;
		mips_[mip].height = height;
		mips_[mip].depth = depth;
		mips_[mip].row_pitch = blocks_x * block_stride_;
		mips_[mip].slice_pitch = blocks_x * blocks_y * block_stride_;

		size_t mip_size = mips_[mip].slice_pitch * array_size_ * depth;

		offset += mip_size;
		width = std::max((width >> 1u), 1u);
		height = std::max((height >> 1u), 1u);
		depth = std::max((depth >> 1u), 1u);
	}

	required_size_ = offset;
}

}