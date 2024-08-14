#pragma once
#include <glm/glm.hpp>
#include "Quark/Asset/Asset.h"
#include "Quark/Asset/Texture.h"

namespace quark {
enum class AlphaMode {
    OPAQUE,
    TRANSPARENT
};

struct Material : public Asset {
    struct UniformBufferBlock {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metalicFactor = 1.f;
        float roughNessFactor = 1.f;
    } uniformBufferData;

    AlphaMode alphaMode = AlphaMode::OPAQUE;

    Ref<graphic::Buffer> uniformBuffer;
    size_t uniformBufferOffset = 0;

    Ref<Texture> baseColorTexture;
    Ref<Texture> metallicRoughnessTexture;
    Ref<Texture> normalTexture;
};

}