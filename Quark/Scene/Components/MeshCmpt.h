#pragma once
#include "Quark/Ecs/Entity.h"
#include "Quark/Scene/Resources/Mesh.h"

namespace quark {
struct MeshCmpt : public Component {
    // Use mesh if it exists, otherwise use sharedMesh
    Scope<Mesh> mesh;
    Ref<Mesh> sharedMesh;
    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};

}