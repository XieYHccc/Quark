#pragma once 
#include "Quark/Asset/Asset.h"
#include "Quark/RHI/Image.h"

namespace quark {

struct Texture : public Asset {
    Ref<rhi::Image> image;
    Ref<rhi::Sampler> sampler;
};

}