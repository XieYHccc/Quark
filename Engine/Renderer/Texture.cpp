#include "pch.h"
#include <stb_image.h>
#include "Renderer/Texture.h"
#include "Renderer/Renderer.h"

namespace asset {

std::unordered_map<std::string, std::shared_ptr<Sampler>> Sampler::assetPool_;
std::unordered_map<std::string, std::shared_ptr<Texture>> Texture::assetPool_;

std::shared_ptr<Sampler> Sampler::AddToPool(const SamplerCreateInfo &info)
{
    std::string uuid = info.path + ":" + info.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Sampler>(info);
    return assetPool_[uuid];
    
}

std::shared_ptr<Sampler> Sampler::GetFromPool(const std::string &path, const std::string &name)
{
    std::string uuid = path+ ":" + name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }

    CORE_LOGW(" Sampler Asset with path {} and name {} doesn't exist", path , name);
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
    std::string uuid = info.path + ":" + info.name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }
    // mesh doesn't exsit, create it.
    assetPool_[uuid] = std::make_shared<Texture>(info);
    return assetPool_[uuid];
}

std::shared_ptr<Texture> Texture::GetFromPool(const std::string &path, const std::string &name)
{
    std::string uuid = path + ":" + name;

    auto find = assetPool_.find(uuid);
    if (find != assetPool_.end()) {
        return find->second;
    }

    CORE_LOGW(" Texture Asset with path {} and name {} doesn't exist", path, name);
    return nullptr;
}

Texture::Texture(const TextureCreateInfo& info)
    : BaseAsset(info.path, info.name)
{
    image = vk::Image::CreateTexImage(Renderer::Instance().GetContext(), info.data, info.width, info.height, info.mipmapped);

}

Texture::Texture(const std::string& filepath)
    : BaseAsset(filepath, "")
{
    TextureCreateInfo info;
    int width, height, nChannels;

    info.data = stbi_load(filepath.c_str(), &width, &height, &nChannels, 0);
    if (info.data) {
        info.width = width;
        info.height = height;
        info.nChannels = nChannels;

        image = vk::Image::CreateTexImage(Renderer::Instance().GetContext(), info.data, info.width, info.height, info.mipmapped);\

        stbi_image_free(info.data);
    }
    else {
        CORE_LOGE("Texture failed to load at path", filepath)
    }

}

Texture::~Texture()
{
    vk::Image::DestroyImage(Renderer::Instance().GetContext(), image);

}

}