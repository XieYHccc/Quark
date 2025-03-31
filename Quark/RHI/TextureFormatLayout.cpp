#include "Quark/qkpch.h"
#include "Quark/RHI/TextureFormatLayout.h"

namespace quark::rhi {

void TextureFormatLayout::SetUp2D(DataFormat format, uint32_t width, uint32_t height, uint32_t array_size, uint32_t mip_levels)
{
    m_format = format;
    m_image_type = ImageType::TYPE_2D;
    m_array_size = array_size;
    m_mip_levels = mip_levels;
    
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
	m_block_stride = GetFormatStride(m_format);
	GetFormatBlockDim(m_format, m_block_dim_x, m_block_dim_y);

    // generate mipmaps
	if (m_mip_levels == 0)
        m_mip_levels = GeneratedMipCount(width, height, depth);

	size_t offset = 0;
	for (uint32_t mip = 0; mip < m_mip_levels; mip++)
	{
		offset = (offset + 15) & ~15;

		uint32_t blocks_x = (width + m_block_dim_x - 1) / m_block_dim_x;
		uint32_t blocks_y = (height + m_block_dim_y - 1) / m_block_dim_y;

		m_mips[mip].offset = offset;
		m_mips[mip].num_block_x = blocks_x;
		m_mips[mip].num_block_y = blocks_y;
		m_mips[mip].row_length = blocks_x * m_block_dim_x;
		m_mips[mip].image_height = blocks_y * m_block_dim_y;
		m_mips[mip].width = width;
		m_mips[mip].height = height;
		m_mips[mip].depth = depth;
		m_mips[mip].row_pitch = blocks_x * m_block_stride;
		m_mips[mip].slice_pitch = blocks_x * blocks_y * m_block_stride;

		size_t mip_size = m_mips[mip].slice_pitch * m_array_size * depth;

		offset += mip_size;
		width = std::max((width >> 1u), 1u);
		height = std::max((height >> 1u), 1u);
		depth = std::max((depth >> 1u), 1u);
	}

	m_required_size = offset;
}

}