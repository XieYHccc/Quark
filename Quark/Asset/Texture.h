#pragma once 
#include "Quark/Asset/Asset.h"
#include "Quark/Graphic/Image.h"

namespace quark {

struct Texture : public Asset {
    Ref<graphic::Image> image;
    Ref<graphic::Sampler> sampler;
};

}