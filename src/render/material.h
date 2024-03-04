#pragma once

#include <string>
#include <vector>
#include <memory>

#include "./texture2d.h"

struct Material {
    std::vector<std::pair<std::string,std::shared_ptr<Texture2D>>> textures;

    Material() {}
    ~Material() {}

    static std::shared_ptr<Material> load_from_mtl(const std::string& path);
};
