#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/MeshAsset.h"

namespace quark {
struct MeshCmpt : public Component {
    // Use unique mesh if it exists, otherwise use sharedMesh
    Ref<MeshAsset> sharedMesh;
    Ref<MeshAsset> uniqueMesh;

    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};

}