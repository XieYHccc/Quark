#pragma once
#include <glm/glm.hpp>
#include "Quark/Scene/Resources/Base.h"
#include "Quark/Scene/Resources/Texture.h"
#include "Quark/Graphic/Common.h"

namespace quark {

struct Material : public Resource{
    enum AlphaMode {
        OPAQUE,
        TRANSPARENT
    } alphaMode = AlphaMode::OPAQUE;

    struct UniformBufferBlock {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metalicFactor = 1.f;
        float roughNessFactor = 1.f;
    } uniformBufferData;

    // Gpu resources
    Ref<graphic::Buffer> uniformBuffer = nullptr;
    size_t uniformBufferOffset = 0;

    Ref<Texture> baseColorTexture;
    Ref<Texture> metallicRoughnessTexture;
    Ref<Texture> normalTexture;
};
}