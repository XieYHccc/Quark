#include "pch.h"
#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"

namespace asset {

std::unordered_map<std::string, std::shared_ptr<Mesh>> Mesh::assetPool_ = {};

std::shared_ptr<Mesh> Mesh::AddToPool(const MeshCreateInfo &createInfo)

{
    std::string uuid = createInfo.path.string() + ":" + createInfo.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Mesh>(createInfo);
    return assetPool_[uuid];
    
}


Mesh::Mesh(const MeshCreateInfo& createInfo)
    : BaseAsset(createInfo.path, createInfo.path)
{

	const size_t vertexBufferSize = createInfo.vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = createInfo.indices.size() * sizeof(uint32_t);

	// create vertex buffer
    vk::BufferBuilder builder;
    builder.SetSize(vertexBufferSize)
        .SetUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .SetVmaUsage(VMA_MEMORY_USAGE_GPU_ONLY);
	meshBuffers_.vertexBuffer = builder.Build(Renderer::Instance().GetContext());
	// find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = meshBuffers_.vertexBuffer.vkBuffer };
	meshBuffers_.vertexBufferAddress = vkGetBufferDeviceAddress(Renderer::Instance().GetContext().GetVkDevice(), &deviceAdressInfo);

	// create index buffer
    builder.Clear();
    builder.SetSize(indexBufferSize)
        .SetUsage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        .SetVmaUsage(VMA_MEMORY_USAGE_GPU_ONLY);
	meshBuffers_.indexBuffer = builder.Build(Renderer::Instance().GetContext());

	// create buffer for copying
	vk::Buffer staging = vk::Buffer::CreateStagingBuffer(Renderer::Instance().GetContext(), vertexBufferSize + indexBufferSize);
	void* data = staging.info.pMappedData;

	// copy vertex buffer
	memcpy(data, createInfo.vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, createInfo.indices.data(), indexBufferSize);

	Renderer::Instance().GetContext().ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;
		vkCmdCopyBuffer(cmd, staging.vkBuffer, meshBuffers_.vertexBuffer.vkBuffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;
		vkCmdCopyBuffer(cmd, staging.vkBuffer, meshBuffers_.indexBuffer.vkBuffer, 1, &indexCopy);
	});

	vk::Buffer::DestroyBuffer(Renderer::Instance().GetContext(), staging);

    // Set submeshes
    submeshs_ = createInfo.submeshs;
	dynamic_ = createInfo.dynamic;
}

Mesh::~Mesh()
{
    vk::Buffer::DestroyBuffer(Renderer::Instance().GetContext(),meshBuffers_.vertexBuffer);
    vk::Buffer::DestroyBuffer(Renderer::Instance().GetContext(),meshBuffers_.indexBuffer);
}

}