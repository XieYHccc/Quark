#include "Renderer/Material.h"
#include "Renderer/Renderer.h"
#include "Graphics/Vulkan/Descriptor.h"

namespace asset {

std::unordered_map<std::string, std::shared_ptr<Material>> Material::assetPool_;

Material::Material(const MaterialCreateInfo& info)
    : BaseAsset(info.path, info.name)
{
    this->info_ = info;
    UpdateMaterialSet();
}

Material::~Material()
{
    vk::Buffer::DestroyBuffer(Renderer::Instance().GetContext(), dataBuffer_);
}

std::shared_ptr<Material> Material::AddToPool(const MaterialCreateInfo &info)
{
    std::string uuid = info.path + ":" + info.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Material>(info);
    return assetPool_[uuid];
}

std::shared_ptr<Material> Material::GetFromPool(const std::string& path, const std::string& name)
{
    std::string uuid = path + ":" + name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }

    CORE_LOG_WARN(" Texture Asset with path {} and name {} doesn't exist", path, name);
    return nullptr;
}

void Material::UpdateMaterialSet()
{
    if (materialSet_ == VK_NULL_HANDLE) {
        materialSet_ = Renderer::Instance().CreateMaterialDescriptorSet();

        vk::BufferBuilder bufferBuilder;
        bufferBuilder.SetSize(sizeof(asset::MaterialBufferData))
            .SetUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		    .SetVmaUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
		    .SetVmaFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT);
        dataBuffer_ = bufferBuilder.Build(Renderer::Instance().GetContext());
    }

    // update vkbuffer
    MaterialBufferData* bufferData = (MaterialBufferData*)dataBuffer_.info.pMappedData;
    *bufferData = info_.bufferData;

    // update descriptor set
    vk::DescriptorWriter writer;
	writer.WriteBuffer(0, dataBuffer_.vkBuffer, VK_WHOLE_SIZE, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, info_.baseColorTex.texture->image.imageView, info_.baseColorTex.sampler->vksampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, info_.metalRougthTex.texture->image.imageView, info_.metalRougthTex.sampler->vksampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(Renderer::Instance().GetContext(), materialSet_);
}


}