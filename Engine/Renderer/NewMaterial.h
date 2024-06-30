#pragma once
#include <glm/glm.hpp>
#include "Graphic/Common.h"

namespace render {

struct Texture {
    Ref<graphic::Image> image;
    Ref<graphic::Sampler> sampler;
};

struct Material {
    enum class AlphaMode {
        OPAQUE,
        TRANSPARENT
    };
    AlphaMode alphaMode = AlphaMode::OPAQUE;
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    Texture baseColorTexture;
    Texture metallicRoughnessTexture;
    Texture normalTexture;


};
}