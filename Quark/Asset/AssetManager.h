#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Graphic/Common.h"

namespace quark {

struct Asset {
    enum class Type {
        MESH,
        TEXTURE,
        MATERIAL,
        SHADER,
        MAX_ENUM
    };

    Ref<graphic::Image> image;
    std::string fileName;

};


class AssetManager {

};

}