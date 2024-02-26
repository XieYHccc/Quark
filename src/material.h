#pragma once

#include <string>
#include <vector>
#include <map>

#include "texture.h"

struct Material {
    std::vector<std::pair<std::string,Texture*>> textures;

    Material() {}

    ~Material() {
        
    }
};
