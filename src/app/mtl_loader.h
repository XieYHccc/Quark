#pragma once

#include <string>

class Material;
class MtlLoader {
public:
    MtlLoader() {}

public:
    Material* load_mtl(std::string path);
};