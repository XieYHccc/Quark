#include "pch.h"
#include "Renderer/Texture.h"
#include "Renderer/Renderer.h"

namespace asset {

std::unordered_map<std::string, std::shared_ptr<Sampler>> Sampler::assetPool_;
std::unordered_map<std::string, std::shared_ptr<Texture>> Texture::assetPool_;

std::shared_ptr<Sampler> Sampler::AddToPool(const SamplerCreateInfo &info)
{
    std::string uuid = info.path.string() + ":" + info.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Sampler>(info);
    return assetPool_[uuid];
    
}

std::shared_ptr<Sampler> Sampler::GetFromPool(const AssetFilePath &path, const std::string &name)
{
    std::string uuid = path.string() + ":" + name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }

    UE_CORE_WARN(" Sampler Asset with path {} and name {} doesn't exist", path.string() , name);
    return nullptr;
}

Sampler::Sampler(const SamplerCreateInfo& info)
    : BaseAsset(info.path, info.name)
{
    vkCreateSampler(Renderer::Instance().GetVkDevice(), &info.info, nullptr, &vksampler);
}

Sampler::~Sampler()
{
    vkDestroySampler(Renderer::Instance().GetVkDevice(), vksampler, nullptr);
}


std::shared_ptr<Texture> Texture::AddToPool(const TextureCreateInfo& info)
{
    std::string uuid = info.path.string() + ":" + info.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Texture>(info);
    return assetPool_[uuid];
}

std::shared_ptr<Texture> Texture::GetFromPool(const AssetFilePath &path, const std::string &name)
{
    std::string uuid = path.string() + ":" + name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }

    UE_CLINET_WARN(" Texture Asset with path {} and name {} doesn't exist", path.string() , name);
    return nullptr;
}

Texture::Texture(const TextureCreateInfo& info)
    : BaseAsset(info.path, info.name)
{
    image = vk::Image::CreateTexImage(Renderer::Instance().GetContext(), info.data, info.width, info.height, info.mipmapped);

}

Texture::~Texture()
{
    vk::Image::DestroyImage(Renderer::Instance().GetContext(), image);

}

}