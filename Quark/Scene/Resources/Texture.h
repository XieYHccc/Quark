#pragma once
#include "Quark/Scene/Resources/Base.h"
#include "Quark/Graphic/Common.h"

namespace quark {

struct Texture : public Resource{
    Ref<graphic::Image> image;
    Ref<graphic::Sampler> sampler;
};

}