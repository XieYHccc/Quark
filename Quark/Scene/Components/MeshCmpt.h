#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Asset/Mesh.h"

namespace quark {
struct MeshCmpt : public Component {
    // Use mesh if it exists, otherwise use sharedMesh
    //Ref<Mesh> sharedMesh;
    Ref<Mesh> sharedMesh;
    Ref<Mesh> uniqueMesh;
    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};

}