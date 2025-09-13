#pragma once
#include "Quark/Render/IRenderable.h"
#include "Quark/Render/Material.h"
#include "Quark/Asset/ImageAsset.h"
#include "Quark/RHI/Common.h"

namespace quark
{

struct SkyboxPerDrawCallData
{
	const rhi::Image* cubemap;
	glm::vec3 color;
};

class Skybox : public IRenderable
{
public:
	Skybox() = default;

	void GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform,
		RenderQueue& queue) const override;
	void SetCubemap(Ref<rhi::Image> cubemap) { m_cubemap = cubemap; }
	void SetColor(const glm::vec3& color) { m_color = color; }

private:
	Ref<rhi::Image> m_cubemap;
	glm::vec3 m_color;
};

}