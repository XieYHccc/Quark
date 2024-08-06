#include "qkpch.h"
#include "Scene/Resources/Mesh.h"
#include "Core/Application.h"

namespace scene {

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMeshDescriptor>& subMeshes, bool isDynamic)
    : vertices(vertices), indices(indices), subMeshes(subMeshes), isDynamic(isDynamic)
{   
    CORE_DEBUG_ASSERT(!vertices.empty() && ! indices.empty())
    auto* graphic_device = Application::Instance().GetGraphicDevice();

    // Calculate mesh's aabb from submesh's aabb
    for (auto& submesh : this->subMeshes) {
        if (submesh.aabb.IsValid()) {
            aabb += submesh.aabb;
            continue;
        }

        submesh.aabb = {};
        for (size_t i = 0; i < submesh.count; i ++) {
            const Vertex& v = vertices[indices[submesh.startIndex + i]];
            submesh.aabb += v.position;
        }
        aabb += submesh.aabb;

        CORE_DEBUG_ASSERT(submesh.aabb.Min() != submesh.aabb.Max())
    }

    CreateRenderResources();
}

void Mesh::CreateRenderResources()
{
    // Delete Render resources if they already exist
    if (vertexBuffer)
        vertexBuffer.reset();
    if (indexBuffer)
        indexBuffer.reset();

	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);
    auto* graphic_device = Application::Instance().GetGraphicDevice();

    // Create vertex buffer
    graphic::BufferDesc vert_buffer_desc;
    vert_buffer_desc.domain = (isDynamic ? graphic::BufferMemoryDomain::CPU : graphic::BufferMemoryDomain::GPU);
    vert_buffer_desc.size = vertexBufferSize;
    vert_buffer_desc.usageBits = graphic::BUFFER_USAGE_VERTEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT;
    vertexBuffer = graphic_device->CreateBuffer(vert_buffer_desc, vertices.data());

    // Create index buffer
    graphic::BufferDesc index_buffer_desc;
    index_buffer_desc.domain = graphic::BufferMemoryDomain::GPU;
    index_buffer_desc.size = indexBufferSize;
    index_buffer_desc.usageBits = graphic::BUFFER_USAGE_INDEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT;
    indexBuffer = graphic_device->CreateBuffer(index_buffer_desc, indices.data());
}

}