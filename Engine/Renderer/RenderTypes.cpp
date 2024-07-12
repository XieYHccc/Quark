#include "pch.h"
#include "Renderer/RenderTypes.h"
#include "Core/Application.h"

namespace render {
Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<u32> indices, const std::vector<SubMeshDescriptor>& submeshes)
    : subMeshes(submeshes)
{   
    CORE_DEBUG_ASSERT(!indices.empty()) //TODO: Support non-indexed mesh
    auto* graphic_device = Application::Instance().GetGraphicDevice();

    // Create vertex buffer and index buffer
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    graphic::BufferDesc vert_buffer_desc = {
        .domain = graphic::BufferMemoryDomain::GPU,
        .size = vertexBufferSize,
        .type = graphic::BufferType::VERTEX_BUFFER
    };

    graphic::BufferDesc index_buffer_desc = {
        .domain = graphic::BufferMemoryDomain::GPU,
        .size = indexBufferSize,
        .type = graphic::BufferType::INDEX_BUFFER
    };

    vertexBuffer = graphic_device->CreateBuffer(vert_buffer_desc, vertices.data());
    indexBuffer = graphic_device->CreateBuffer(index_buffer_desc, indices.data());

    // Calculate mesh's aabb from submesh's aabb
    for (auto& submesh : subMeshes) {
        if (submesh.aabb.is_valid()) {
            aabb += submesh.aabb;
            continue;
        }

        submesh.aabb = {};
        for (size_t i = 0; i < submesh.count; i ++) {
            const Vertex& v = vertices[indices[submesh.startIndex + i]];
            submesh.aabb += v.position;
        }
        aabb += submesh.aabb;

        CORE_DEBUG_ASSERT(submesh.aabb.min() != submesh.aabb.max())
    }

}
}