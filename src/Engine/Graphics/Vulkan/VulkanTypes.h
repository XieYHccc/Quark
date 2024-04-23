#pragma once
#include "pch.h"

#include <glm/glm.hpp>

enum class GPU_BUFFER_TYPE {
    INVALID,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    UNIFOR_BUFFER

};

enum class GPU_IMAGE_TYPE {
    INVALID,
    TEXTURE,
    COLOR_FRAME_BUFFER,
    DEPTH_FRAME_BUFFER
};

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

struct GpuBufferVulkan {

    // vulkan objects
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;

    GPU_BUFFER_TYPE type;
};

struct GpuImageVulkan {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
    GPU_IMAGE_TYPE type;
};

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
    GpuBufferVulkan indexBuffer;
    GpuBufferVulkan vertexBuffer;
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



