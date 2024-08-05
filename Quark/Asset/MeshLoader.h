#pragma once
#include "Graphic/Device.h"
#include "Scene/Resources/Mesh.h"

namespace asset {

class MeshLoader {
public:
    MeshLoader(graphic::Device* device) : graphicDevice_(device) {};

    Ref<scene::Mesh> LoadGLTF(const std::string& filepath);
    Ref<scene::Mesh> LoadOBJ(const std::string& filepath);
private:
    graphic::Device* graphicDevice_;
};
}