#pragma once
#include "pch.h"

#include <glm/glm.hpp>

enum class GPU_PIPELINE_TYPE {
    INVALID,
    GRAPHICS,
    COMPUTING
};

enum class MATERIAL_PASS_TYPE :uint8_t {
    INVALID,
    OPAQUE,
    TRANSPARENT
};

struct AllocatedBuffer {
    // vulkan objects
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

using UniformBuffer = AllocatedBuffer;
using VertexBuffer = AllocatedBuffer;
using IndexBuffer = AllocatedBuffer;

using Texture = AllocatedImage;
using ColorAttachment = AllocatedImage;
using DepthAttachment = AllocatedImage;

struct GpuPipeLineVulkan {

    VkPipeline pipeline;
    VkPipelineLayout layout;
    GPU_PIPELINE_TYPE type;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
    const char* name;
    GpuPipeLineVulkan computePipeline;
	ComputePushConstants data;
};

struct GpuSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct Bounds {
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

struct Vertex {

	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

// submesh's material
struct GpuMaterialInstance {
    GpuPipeLineVulkan* pipeline;
    VkDescriptorSet materialSet;
    MATERIAL_PASS_TYPE passType;
};

struct GpuDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

struct SubMeshDescriptor {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    GpuMaterialInstance* material;
};

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;

	GpuMaterialInstance* material;
    Bounds bounds;
	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
};

struct GpuMeshBuffers {
    IndexBuffer indexBuffer;
    VertexBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

struct GpuMeshVulkan {
    std::string name;
    GpuMeshBuffers meshBuffers;
    std::vector<SubMeshDescriptor> submeshes;
    bool Dynamic {false};
};

struct DrawContext {
	std::vector<RenderObject> OpaqueObjects;
    std::vector<RenderObject> TransparentObjects;

    GpuSceneData sceneData;
};



