#pragma once
#include "Quark/Render/IRenderable.h"
#include "Quark/RHI/Common.h"
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Asset/MaterialAsset.h"

#include <glm/glm.hpp>

namespace quark
{
class ShaderProgram;

enum class TextureKind : unsigned
{
	Albedo = 0,
	Normal = 1,
	MetallicRoughness = 2,
	Occlusion = 3,
	Emissive = 4,
	Count
};

enum MaterialTextureFlagBits
{
	MATERIAL_TEXTURE_BASE_COLOR_BIT = 1u << util::ecast(TextureKind::Albedo),
	MATERIAL_TEXTURE_NORMAL_BIT = 1u << util::ecast(TextureKind::Normal),
	MATERIAL_TEXTURE_METALLIC_ROUGHNESS_BIT = 1u << util::ecast(TextureKind::MetallicRoughness),
	MATERIAL_TEXTURE_OCCLUSION_BIT = 1u << util::ecast(TextureKind::Occlusion),
	MATERIAL_TEXTURE_EMISSIVE_BIT = 1u << util::ecast(TextureKind::Emissive),
	MATERIAL_EMISSIVE_BIT = 1u << 5,
	MATERIAL_EMISSIVE_REFRACTION_BIT = 1u << 6,
	MATERIAL_EMISSIVE_REFLECTION_BIT = 1u << 7
};

struct PBRMaterial
{
	Ref<rhi::Image> textures[util::ecast(TextureKind::Count)];

	glm::vec4 base_color = glm::vec4(1.f);
	glm::vec3 emissive_color = glm::vec3(0.f);

	float metallic_factor = 1.f;
	float roughness_factor = 1.f;
	bool two_sided = false;

	DrawPipeline draw_pipeline = DrawPipeline::Opaque;
	ShaderProgram* shader_program = nullptr;
	uint64_t hash = 0;

	uint32_t GetTextureMask() const
	{
		uint32_t mask = 0;
		for (unsigned i = 0; i < util::ecast(TextureKind::Count); i++)
		{
			if (textures[i])
				mask |= 1u << i;
		}
		return mask;
	}
};
}