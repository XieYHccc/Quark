#pragma once
#include "Graphic/Device.h"
#include "Renderer/RenderTypes.h"

namespace asset {

class MeshLoader {
public:
    MeshLoader(graphic::Device* device) : graphicDevice_(device) {};

    Ref<render::Mesh> LoadGLTF(const std::string& filepath);
    Ref<render::Mesh> LoadOBJ(const std::string& filepath);
private:
    graphic::Device* graphicDevice_;
};
}