#pragma once
#include "Quark/Graphic/Device.h"
#include "Quark/Scene/Resources/Mesh.h"

namespace quark {

class MeshLoader {
public:
    MeshLoader(graphic::Device* device) : graphicDevice_(device) {};

    Ref<Mesh> LoadGLTF(const std::string& filepath);
    Ref<Mesh> LoadOBJ(const std::string& filepath);
private:
    graphic::Device* graphicDevice_;
};
}