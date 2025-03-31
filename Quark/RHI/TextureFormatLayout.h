#pragma once
#include "Quark/RHI/Common.h"
#include "Quark/RHI/Image.h"
namespace quark::rhi {
// This class fills up texture(sampled image)'s mipmap layout and copy informations
// Designed for copy and blit operations
class TextureFormatLayout {
public:
	struct MipInfo {
		size_t offset = 0;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;

		uint32_t num_block_x = 0;
		uint32_t num_block_y = 0;
		uint32_t row_length = 0;
		uint32_t image_height = 0;
        uint32_t slice_pitch = 0;
        uint32_t row_pitch = 0;
	};

    TextureFormatLayout() = default;
    void SetUp1D();

    // Miplevels = 0 means generate mipmaps automatically
    void SetUp2D(DataFormat format, uint32_t width, uint32_t height, uint32_t array_size_, uint32_t mip_levels);

    uint32_t GetBlockDimX() const { return m_block_dim_x; }
    uint32_t GetBlockDimY() const { return m_block_dim_y; }
    uint32_t GetBlockStride() const { return m_block_stride; }
    uint64_t GetRequiredSize() const { return m_required_size;}
    uint32_t GetMipLevels() const { return m_mip_levels; }
    uint32_t GetArraySize() const { return m_array_size; }
    const MipInfo& GetMipInfo(uint32_t mip_level) const { return m_mips[mip_level];}
    
    static uint32_t GeneratedMipCount(uint32_t width, uint32_t height, uint32_t depth);
private:
    void FillMipInfos(uint32_t width, uint32_t height, uint32_t depth);
    DataFormat m_format = DataFormat::UNDEFINED;
    ImageType m_image_type = ImageType::TYPE_2D;
    size_t m_required_size = 0; // required data source size
    uint32_t m_block_stride = 1;
	uint32_t m_mip_levels = 1;
	uint32_t m_array_size = 1;
	uint32_t m_block_dim_x = 1;
	uint32_t m_block_dim_y = 1;
    MipInfo m_mips[16];
};
}