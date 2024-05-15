#pragma once

#include <glm/glm.hpp>
#include "Renderer/Texture.h"
#include "Graphics/Vulkan/Buffer.h"



enum class AlphaMode :uint8_t {
    INVALID,
    OPAQUE,
    TRANSPARENT
};

namespace asset {

struct SampledTexture {
	std::shared_ptr<Texture> texture;
	std::shared_ptr<Sampler> sampler;
};

struct MaterialBufferData
{
	glm::vec4 colorFactors;
	glm::vec4 metalRoughFactors;
	//padding, we need it anyway for uniform buffers
	glm::vec4 extra[14];
};

struct MaterialCreateInfo {
	std::string path;
	std::string name;

	MaterialBufferData bufferData;
	SampledTexture baseColorTex;
	SampledTexture metalRougthTex;
	AlphaMode mode;

};

class Material : public BaseAsset {
    static  std::unordered_map<std::string, std::shared_ptr<Material>> assetPool_;
public: 
    static std::shared_ptr<Material> AddToPool(const MaterialCreateInfo& info);
	static std::shared_ptr<Material> GetFromPool(const std::string& path, const std::string& name);
    static void ClearPool() { assetPool_.clear();}

	Material(const MaterialCreateInfo& info);
	~Material();
	ASSET_TYPE("Renderer::Material")

	AlphaMode GetAlphaMode() const { return info_.mode; }
	VkDescriptorSet& GetMaterialSet() { return materialSet_; }

public:
	void UpdateMaterialSet();

	vk::Buffer dataBuffer_;
	VkDescriptorSet materialSet_;
	MaterialCreateInfo info_;

};

}