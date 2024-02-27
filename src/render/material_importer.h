#pragma once

#include <string>

struct Material;
class MaterialImporter {
public:
    MaterialImporter() {};

public:
    Material* load_from_mtl(std::string obj_path);
    
};