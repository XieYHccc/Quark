#pragma once
#include "Quark/RHI/Device.h"
#include "Quark/Asset/MeshAsset.h"

namespace quark {
class MeshImporter {
public:
    MeshImporter() = default;

    Ref<MeshAsset> ImportGLTF(const std::string& filepath);
    Ref<MeshAsset> ImportOBJ(const std::string& filepath);
};
}