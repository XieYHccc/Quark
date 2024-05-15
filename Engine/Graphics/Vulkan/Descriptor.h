#pragma once

#include <deque>
#include <span>

#include "Graphics/Vulkan/Context.h"

namespace vk {

class DescriptorSetLayoutBuilder {
public:
	VkDescriptorSetLayout Build(Context& context);
    void AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages);
    void Clear();

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkShaderStageFlags shaderStages;
};


class DescriptorAllocator {
public:
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

public:

	void Init(Context& context, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
	void ClearPools();
	void DestroyPools();

	VkDescriptorSet Allocate( VkDescriptorSetLayout layout);

private:
	Context* context_;
	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools;
	std::vector<VkDescriptorPool> readyPools;
	uint32_t setsPerPool;

private:
	VkDescriptorPool GetPool();
	VkDescriptorPool CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
};

class DescriptorWriter {
public:
	
    void WriteImage(int binding,VkImageView image,VkSampler sampler , VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(int binding,VkBuffer buffer,size_t size, size_t offset,VkDescriptorType type); 

    void Clear();
    void UpdateSet(Context& context, VkDescriptorSet set);

private:
    std::deque<VkDescriptorImageInfo> imageInfos_;
    std::deque<VkDescriptorBufferInfo> bufferInfos_;
    std::vector<VkWriteDescriptorSet> writes_;
};

}