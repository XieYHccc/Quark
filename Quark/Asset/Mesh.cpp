#include "Quark/Asset/Mesh.h"
#include "Quark/Core/Application.h"

namespace quark {
Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMeshDescriptor>& subMeshes, bool isDynamic)
    : vertices(vertices), indices(indices), subMeshes(subMeshes), isDynamic(isDynamic)
{   
    CORE_DEBUG_ASSERT(!vertices.empty() && ! indices.empty())
    auto* graphic_device = Application::Get().GetGraphicDevice();

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

    UpdateGpuBuffers();
}

uint32_t Mesh::GetMeshAttributeMask() const
{
    uint32_t result = 0;

    if (!positions.empty())
		result |= MESH_ATTRIBUTE_POSITION_BIT;
    if (!uvs.empty())
        result |= MESH_ATTRIBUTE_UV_BIT;
    if (!normals.empty())
        result |= MESH_ATTRIBUTE_NORMAL_BIT;
    if (!colors.empty())
        result |= MESH_ATTRIBUTE_VERTEX_COLOR_BIT;

    return result;
}

void Mesh::UpdateGpuBuffers()
{
    auto* graphic_device = Application::Get().GetGraphicDevice();

    // Prepare overlapped data
    size_t vertexNum = positions.size();
    size_t stride = 0;

    stride += sizeof(decltype(positions)::value_type);
    if (!uvs.empty()) stride += sizeof(decltype(uvs)::value_type);
    if (!normals.empty()) stride += sizeof(decltype(normals)::value_type);
    if (!colors.empty()) stride += sizeof(decltype(colors)::value_type);
    m_OverlappedVertexData.resize(stride * vertexNum);

    size_t offset = 0;
    uint8_t* data = m_OverlappedVertexData.data();
    for (size_t i = 0; i < vertexNum; ++i)
    {
        memcpy(data + offset, &positions[i], sizeof(decltype(positions)::value_type));
        offset += sizeof(decltype(positions)::value_type);

        if (!uvs.empty())
        {
            memcpy(data + offset, &uvs[i], sizeof(decltype(uvs)::value_type));
			offset += sizeof(decltype(uvs)::value_type);
		}

        if (!normals.empty())
		{
			memcpy(data + offset, &normals[i], sizeof(decltype(normals)::value_type));
			offset += sizeof(decltype(normals)::value_type);
        }

        if (!colors.empty())
        {
            memcpy(data + offset, &colors[i], sizeof(decltype(colors)::value_type));
			offset += sizeof(decltype(colors)::value_type);
        }

    }

    // Update vertex buffer
    if (!m_VertexBuffer)
    {
        const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);

        graphic::BufferDesc vert_buffer_desc;
        vert_buffer_desc.domain = (isDynamic ? graphic::BufferMemoryDomain::CPU : graphic::BufferMemoryDomain::GPU);
        vert_buffer_desc.size = vertexBufferSize;
        vert_buffer_desc.usageBits = graphic::BUFFER_USAGE_VERTEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT;
        m_VertexBuffer = graphic_device->CreateBuffer(vert_buffer_desc, vertices.data());
    }
    else
    {   // Only dynamic mesh can be updated
        CORE_ASSERT(isDynamic && m_VertexBuffer->GetDesc().domain == graphic::BufferMemoryDomain::CPU)

        void* mappedData = m_VertexBuffer->GetMappedDataPtr();
        memcpy(mappedData, vertices.data(), vertices.size() * sizeof(Vertex));
    }

    // Update index buffer, Currently, we don't support dynamic index buffer
    if (!m_IndexBuffer)
    {
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        graphic::BufferDesc index_buffer_desc;
        index_buffer_desc.domain = graphic::BufferMemoryDomain::GPU;
        index_buffer_desc.size = indexBufferSize;
        index_buffer_desc.usageBits = graphic::BUFFER_USAGE_INDEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT;
        m_IndexBuffer = graphic_device->CreateBuffer(index_buffer_desc, indices.data());
    }
}

}