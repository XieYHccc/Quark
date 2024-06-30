#pragma once
#include "Scene/Ecs.h"
#include "Renderer/Common.h"

namespace scene {
struct MeshCmpt : public Component {
    Ref<render::Mesh> mesh;
    using Component::Component;
};


}