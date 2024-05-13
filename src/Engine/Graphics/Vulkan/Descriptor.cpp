#include "pch.h"
#include "Graphics/Vulkan/Descriptor.h"

namespace vk {

void DescriptorSetLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages)
{
    VkDescriptorSetLayoutBinding newbind {};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;
    newbind.stageFlags |= shaderStages;
    bindings.push_back(newbind);
}

void DescriptorSetLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::Build(Context& context)
{
    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = nullptr;

    info.pBindings = bindings.data();
    info.bindingCount = (uint32_t)bindings.size();
    info.flags = 0;

    VkDescriptorSetLayout set;
    if (vkCreateDescriptorSetLayout(context.GetVkDevice(), &info, nullptr, &set) != VK_SUCCESS)
		UE_CORE_ERROR("DescriptorSetLayoutBuilder::Build() : Failed to create descriptor set layout")

    return set;
}

void DescriptorAllocator::Init(Context& context, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios)
{
    this->context_ = &context;

	ratios.clear();
    
    for (auto r : poolRatios) {
        ratios.push_back(r);
    }
	
    VkDescriptorPool newPool = CreatePool(initialSets, poolRatios);

    setsPerPool = initialSets * 1.5; //grow it next allocation

    readyPools.push_back(newPool);
}

VkDescriptorPool DescriptorAllocator::GetPool()
{       
    VkDescriptorPool newPool;
    if (readyPools.size() != 0) {
        newPool = readyPools.back();
        readyPools.pop_back();
    }
    else {
	    //need to create a new pool
	    newPool = CreatePool(setsPerPool, ratios);

	    setsPerPool = setsPerPool * 1.5;
	    if (setsPerPool > 4092) {
		    setsPerPool = 4092;
	    }
    }   

    return newPool;
}

VkDescriptorPool DescriptorAllocator::CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * setCount)
		});
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(context_->GetVkDevice(), &pool_info, nullptr, &newPool);
    return newPool;
}

void DescriptorAllocator::ClearPools()
{ 
    for (auto p : readyPools) {
        vkResetDescriptorPool(context_->GetVkDevice(), p, 0);
    }
    for (auto p : fullPools) {
        vkResetDescriptorPool(context_->GetVkDevice(), p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocator::DestroyPools()
{
	for (auto p : readyPools) {
		vkDestroyDescriptorPool(context_->GetVkDevice(), p, nullptr);
	}
	for (auto p : fullPools) {
		vkDestroyDescriptorPool(context_->GetVkDevice(),p,nullptr);
    }
    readyPools.clear();
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::Allocate( VkDescriptorSetLayout layout)
{
    //get or create a pool to allocate from
    VkDescriptorPool poolToUse = GetPool();

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = poolToUse;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VkResult result = vkAllocateDescriptorSets(context_->GetVkDevice(), &allocInfo, &ds);

    //allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

        fullPools.push_back(poolToUse);
    
        poolToUse = GetPool();
        allocInfo.descriptorPool = poolToUse;

       vkAllocateDescriptorSets(context_->GetVkDevice(), &allocInfo, &ds);
    }
  
    readyPools.push_back(poolToUse);
    return ds;
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
	VkDescriptorBufferInfo& info = bufferInfos_.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = size
		});

	VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &info;

	writes_.push_back(write);
}

void DescriptorWriter::WriteImage(int binding, VkImageView image, VkSampler sampler,  VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& info = imageInfos_.emplace_back(VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = image,
		.imageLayout = layout
	});

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &info;

	writes_.push_back(write);
}

void DescriptorWriter::Clear()
{
    imageInfos_.clear();
    writes_.clear();
    bufferInfos_.clear();
}

void DescriptorWriter::UpdateSet(Context& context, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes_) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(context.GetVkDevice(), (uint32_t)writes_.size(), writes_.data(), 0, nullptr);
}

}