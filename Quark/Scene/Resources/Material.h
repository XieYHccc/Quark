#pragma once
#include <glm/glm.hpp>
#include "Scene/Resources/Base.h"
#include "Scene/Resources/Texture.h"
#include "Graphic/Common.h"


namespace scene {
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