#pragma once
#include "pch.h"
#include "Scene/Ecs.h"
#include "Scene/Resources/Mesh.h"
namespace scene {
struct MeshCmpt : public Component {
    // Use mesh if it exists, otherwise use sharedMesh
    Scope<Mesh> mesh;
    Ref<Mesh> sharedMesh;
    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};
}