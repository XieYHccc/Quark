#pragma once

#include "Quark/Render/RenderQueue.h"
#include "Quark/Render/RenderContext.h"

namespace quark 
{
struct RenderInfoCmpt;

enum class DrawPipeline : uint8_t
{
	Opaque,
	AlphaTest,
	AlphaBlend,
};

enum class DrawPipelineCoverage : uint8_t
{
    Full,
    Modify,
};


class IRenderable 
{
public:
	virtual ~IRenderable() = default;

	virtual void GetRenderData(const RenderContext& context, const RenderInfoCmpt* transform, RenderQueue& render_queue) const = 0;
	
	virtual bool HasStaticAabb() const { return false;}

	virtual const math::Aabb* GetStaticAabb() const
	{
		static const math::Aabb aabb(glm::vec3(0.0f), glm::vec3(0.0f));
		return &aabb;
	}

	virtual DrawPipeline GetMeshDrawPipeline() const { return DrawPipeline::Opaque; }
};

}
