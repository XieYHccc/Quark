#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Asset/Material.h"

namespace quark {

// Encapsulates the data which is mirrored to render a MeshComponent parallel to the game thread
//struct RenderProxy
//{
//    uint32_t indexCount = 0;
//    uint32_t firstIndex = 0;
//    Ref<graphic::Buffer> indexBuffer;
//    Ref<graphic::Buffer> attributeBuffer;
//    Ref<graphic::Buffer> positionBuffer;
//    Ref<graphic::PipeLine> pipeLine; // for main pass
//    Ref<Material> material;
//    math::Aabb aabb = {};
//    glm::mat4 transform;
//};
//
//struct EditorModeRenderProxy : public RenderProxy
//{
//    uint64_t entityID = 0;
//};
}