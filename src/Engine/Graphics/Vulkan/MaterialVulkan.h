#pragma once
#include "pch.h"

#include <glm/glm.hpp>

#include "Graphics/Vulkan/VulkanTypes.h"
#include "Graphics/Vulkan/DescriptorVulkan.h"


class IMaterialFactory
{
public:
	virtual void BuildPipelines() = 0;
	virtual void ClearResources() = 0;

};

class GLTFMetallic_Roughness : public IMaterialFactory{
public:
	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		GpuImageVulkan colorImage;
		VkSampler colorSampler;
		GpuImageVulkan metalRoughImage;
		VkSampler metalRoughSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};
    
	virtual void BuildPipelines() override;
    virtual void ClearResources() override;

	std::unique_ptr<GpuMaterialInstance> CreateInstance(MATERIAL_PASS_TYPE pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator);

private:
	GpuPipeLineVulkan opaquePipeline;
	GpuPipeLineVulkan transparentPipeline;
	VkDescriptorSetLayout materialLayout;
	DescriptorWriter writer;
};