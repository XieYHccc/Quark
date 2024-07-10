#pragma once
#include "pch.h"
#include "Scene/Ecs.h"
#include "Renderer/RenderTypes.h"

namespace scene {
struct MeshCmpt : public Component {
    Ref<render::Mesh> mesh;
    QK_COMPONENT_TYPE_DECL(MeshCmpt)
    using Component::Component;
};
}