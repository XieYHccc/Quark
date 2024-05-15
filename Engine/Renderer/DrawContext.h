#pragma once
#include "Renderer/Material.h"
#include "Renderer/Bounds.h"

struct SceneBufferData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;
    std::shared_ptr<asset::Material> material;
    Bounds bounds;
	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
};

struct DrawContext {
    std::vector<RenderObject> OpaqueObjects;
    std::vector<RenderObject> TransparentObjects;
    SceneBufferData sceneData;
    
};