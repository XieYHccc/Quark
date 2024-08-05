#pragma once
#include "Scene/Resources/Base.h"
#include "Graphic/Common.h"

namespace scene {
struct Texture : public Resource{
    Ref<graphic::Image> image;
    Ref<graphic::Sampler> sampler;
};
}