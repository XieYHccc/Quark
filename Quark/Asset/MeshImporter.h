#pragma once
#include "Quark/Graphic/Device.h"
#include "Quark/Asset/Mesh.h"

namespace quark {
class MeshImporter {
public:
    MeshImporter() = default;

    Ref<Mesh> ImportGLTF(const std::string& filepath);
    Ref<Mesh> ImportOBJ(const std::string& filepath);
};
}