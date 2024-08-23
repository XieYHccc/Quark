#pragma once
#include "Quark/Graphic/Common.h"
#include "Quark/Graphic/Image.h"
namespace quark::graphic {
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

    uint32_t GetBlockDimX() const { return block_dim_x_; }
    uint32_t GetBlockDimY() const { return block_dim_y_; }
    uint32_t GetBlockStride() const { return block_stride_; }
    uint64_t GetRequiredSize() const { return required_size_;}
    uint32_t GetMipLevels() const { return mip_levels_; }
    uint32_t GetArraySize() const { return array_size_; }
    const MipInfo& GetMipInfo(uint32_t mip_level) const { return mips_[mip_level];}
    
    static uint32_t GeneratedMipCount(uint32_t width, uint32_t height, uint32_t depth);
private:
    void FillMipInfos(uint32_t width, uint32_t height, uint32_t depth);
    DataFormat format_;
    ImageType image_type_;
    size_t required_size_ = 0; // required data source size
    uint32_t block_stride_ = 1;
	uint32_t mip_levels_ = 1;
	uint32_t array_size_ = 1;
	uint32_t block_dim_x_ = 1;
	uint32_t block_dim_y_ = 1;
    MipInfo mips_[16];
};
}