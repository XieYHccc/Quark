#pragma once
#include "Asset/BaseAsset.h"
#include "Graphics/Vulkan/Image.h"

namespace asset {

struct SamplerCreateInfo {
    VkSamplerCreateInfo info;
    AssetFilePath path;
    std::string name;
};

struct Sampler : public BaseAsset{
    static  std::unordered_map<std::string, std::shared_ptr<Sampler>> assetPool_;
public: 
    static std::shared_ptr<Sampler> AddToPool(const SamplerCreateInfo& info);
    static std::shared_ptr<Sampler> GetFromPool(const AssetFilePath& path, const std::string& name);
    static void ClearPool() { assetPool_.clear();}

    VkSampler vksampler;

    Sampler(const SamplerCreateInfo& info);
    ~Sampler();

    ASSET_TYPE("Renderer::Sampler")
};

struct TextureCreateInfo {
    void* data;
    u32 width {0};
    u32 height {0};
    u32 nChannels {0};
    bool mipmapped = false;

    AssetFilePath path;
    std::string name;

};

struct Texture : public BaseAsset {
    static  std::unordered_map<std::string, std::shared_ptr<Texture>> assetPool_;
public: 
    static std::shared_ptr<Texture> AddToPool(const TextureCreateInfo& info);
    static std::shared_ptr<Texture> GetFromPool(const AssetFilePath& path, const std::string& name);
    static void ClearPool() { assetPool_.clear();}

    vk::Image image;

    Texture(const TextureCreateInfo& info);
    ~Texture();

    ASSET_TYPE("Renderer::Texture")
};

}