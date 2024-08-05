#pragma once
#include "Core/Util/Singleton.h"
#include "Graphic/Common.h"

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