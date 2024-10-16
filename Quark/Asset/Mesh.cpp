#include "Quark/Asset/Mesh.h"
#include "Quark/Core/Application.h"

namespace quark {
//Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMeshDescriptor>& subMeshes, bool isDynamic)
//    : vertices(vertices), indices(indices), subMeshes(subMeshes), isDynamic(isDynamic)
//{   
//    QK_CORE_ASSERT(!vertices.empty() && ! indices.empty())
//    auto* graphic_device = Application::Get().GetGraphicDevice();
//
//    // Calculate mesh's aabb from submesh's aabb
//    for (auto& submesh : this->subMeshes) {
//        if (submesh.aabb.IsValid()) {
//            aabb += submesh.aabb;
//            continue;
//        }
//
//        submesh.aabb = {};
//        for (size_t i = 0; i < submesh.count; i ++) {
//            const Vertex& v = vertices[indices[submesh.startIndex + i]];
//            submesh.aabb += v.position;
//        }
//        aabb += submesh.aabb;
//
//        QK_CORE_ASSERT(submesh.aabb.Min() != submesh.aabb.Max())
//    }
//
//    UpdateGpuBuffers();
//}

uint32_t Mesh::GetMeshAttributeMask() const
{
    uint32_t result = 0;

    if (!vertex_positions.empty())
		result |= MESH_ATTRIBUTE_POSITION_BIT;
    if (!vertex_uvs.empty())
        result |= MESH_ATTRIBUTE_UV_BIT;
    if (!vertex_normals.empty())
        result |= MESH_ATTRIBUTE_NORMAL_BIT;
    if (!vertex_colors.empty())
        result |= MESH_ATTRIBUTE_VERTEX_COLOR_BIT;

    return result;
}

size_t Mesh::GetPositionBufferStride() const
{
    return sizeof(decltype(vertex_positions)::value_type);
}

size_t Mesh::GetAttributeBufferStride() const
{
    size_t stride = 0;

    if (!vertex_uvs.empty()) stride += sizeof(decltype(vertex_uvs)::value_type);
    if (!vertex_normals.empty()) stride += sizeof(decltype(vertex_normals)::value_type);
    if (!vertex_colors.empty()) stride += sizeof(decltype(vertex_colors)::value_type);

    return stride;
}

Ref<graphic::Buffer> Mesh::GetAttributeBuffer()
{
    if (!m_attributeBuffer)
        UpdateGpuBuffers();

    return m_attributeBuffer;
}

Ref<graphic::Buffer> Mesh::GetPositionBuffer()
{
    if (!m_positionBuffer)
        UpdateGpuBuffers();

    return m_positionBuffer;
}

Ref<graphic::Buffer> Mesh::GetIndexBuffer()
{
    if (!m_indexBuffer)
        UpdateGpuBuffers();

    return m_indexBuffer;
}

void Mesh::UpdateGpuBuffers()
{
    if (!IsVertexDataArraysValid())
    {
        QK_CORE_LOGE_TAG("Mesh", "Mesh vertex data is invalid, can't update gpu buffers");
		return;
    }

    auto* graphic_device = Application::Get().GetGraphicDevice();

    // Prepare overlapped data
    size_t vertexNum = vertex_positions.size();
    size_t stride = 0;

    if (!vertex_uvs.empty()) stride += sizeof(decltype(vertex_uvs)::value_type);
    if (!vertex_normals.empty()) stride += sizeof(decltype(vertex_normals)::value_type);
    if (!vertex_colors.empty()) stride += sizeof(decltype(vertex_colors)::value_type);
    m_cachedAttributeData.resize(stride * vertexNum);

    size_t offset = 0;
    uint8_t* data = m_cachedAttributeData.data();

    for (size_t i = 0; i < vertexNum; ++i)
    {
        if (!vertex_uvs.empty())
        {
            memcpy(data + offset, &vertex_uvs[i], sizeof(decltype(vertex_uvs)::value_type));
			offset += sizeof(decltype(vertex_uvs)::value_type);
		}

        if (!vertex_normals.empty())
		{
			memcpy(data + offset, &vertex_normals[i], sizeof(decltype(vertex_normals)::value_type));
			offset += sizeof(decltype(vertex_normals)::value_type);
        }

        if (!vertex_colors.empty())
        {
            memcpy(data + offset, &vertex_colors[i], sizeof(decltype(vertex_colors)::value_type));
			offset += sizeof(decltype(vertex_colors)::value_type);
        }

    }

    // Update position buffer
    if (!m_positionBuffer)
    {
        graphic::BufferDesc pos_buffer_desc;
		pos_buffer_desc.domain = (IsDynamic() ? graphic::BufferMemoryDomain::CPU : graphic::BufferMemoryDomain::GPU);
		pos_buffer_desc.size = vertex_positions.size() * sizeof(decltype(vertex_positions)::value_type);
        pos_buffer_desc.usageBits = graphic::BUFFER_USAGE_VERTEX_BUFFER_BIT;
        pos_buffer_desc.usageBits |= (IsDynamic() ? 0 : graphic::BUFFER_USAGE_TRANSFER_TO_BIT);
		m_positionBuffer = graphic_device->CreateBuffer(pos_buffer_desc, vertex_positions.data());
    }

    // Update vertex buffer
    if (!m_attributeBuffer)
    {
        graphic::BufferDesc desc;
        desc.domain = (IsDynamic() ? graphic::BufferMemoryDomain::CPU : graphic::BufferMemoryDomain::GPU);
        desc.size = m_cachedAttributeData.size();
        desc.usageBits = graphic::BUFFER_USAGE_VERTEX_BUFFER_BIT;
        desc.usageBits |= (IsDynamic() ? 0 : graphic::BUFFER_USAGE_TRANSFER_TO_BIT);
        m_attributeBuffer = graphic_device->CreateBuffer(desc, m_cachedAttributeData.data());
    }
    else
    {   // Only dynamic mesh can be updated
        QK_CORE_ASSERT(IsDynamic() && m_attributeBuffer->GetDesc().domain == graphic::BufferMemoryDomain::CPU)

        void* mappedData = m_attributeBuffer->GetMappedDataPtr();
        memcpy(mappedData, m_cachedAttributeData.data(), m_cachedAttributeData.size());
    }

    // Update index buffer, Currently, we don't support dynamic index buffer
    if (!m_indexBuffer)
    {
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        graphic::BufferDesc index_buffer_desc;
        index_buffer_desc.domain = graphic::BufferMemoryDomain::GPU;
        index_buffer_desc.size = indexBufferSize;
        index_buffer_desc.usageBits = graphic::BUFFER_USAGE_INDEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT;
        m_indexBuffer = graphic_device->CreateBuffer(index_buffer_desc, indices.data());
    }
}

bool Mesh::IsVertexDataArraysValid() const
{
    size_t num = vertex_positions.size();
    if (num != 0
        && (vertex_uvs.size() == 0 || vertex_uvs.size() == num)
        && (vertex_normals.size() == 0 || vertex_normals.size() == num)
        && (vertex_colors.size() == 0 || vertex_colors.size() == num) )
        return true;
    else
        return false;
}

void Mesh::CalculateAabbs()
{
    if (vertex_positions.empty())
        return;

    for (auto& submesh : this->subMeshes)
    {
        submesh.aabb = {};
        for (size_t i = 0; i < submesh.count; i++) 
        {
            glm::vec3 position = vertex_positions[indices[submesh.startIndex + i]];
            submesh.aabb += position;
        }

        this->aabb += submesh.aabb;

        QK_CORE_ASSERT(submesh.aabb.Min() != submesh.aabb.Max())
    }
}

}