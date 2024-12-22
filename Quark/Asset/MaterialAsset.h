#pragma once
#include "Quark/Asset/Asset.h"

#include <glm/glm.hpp>

namespace quark 
{
    enum class AlphaMode 
    {
        MODE_OPAQUE,
        MODE_TRANSPARENT
    };

    struct MaterialAsset : public Asset {
        QUARK_ASSET_TYPE_DECL(MATERIAL)

        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metalicFactor = 1.f;
        float roughNessFactor = 1.f;
        AlphaMode alphaMode = AlphaMode::MODE_OPAQUE;

        AssetID baseColorImage = 0;
        AssetID metallicRoughnessImage = 0;
        AssetID normalImage = 0;
        
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

    };
}