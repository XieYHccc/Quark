#pragma once
#include "Quark/RHI/Common.h"

namespace quark::rhi {
    
// In order to keep descriptor set allocator hit rate high, we can make sure that static resources and transient resources
// are kept in separate sets when writing shaders.
// The layout I typically use is:
// - Set 0: Global uniform data (projection matrices and that kind of stuff)
// - Set 1: Global texture resources (like shadow maps, etc)
// - Set 2: Per-material data like textures.
// - Set 3: Per-draw uniforms
class Shader : public GpuResource {
public:
    virtual ~Shader() = default;    

    ShaderStage GetStage() const {return m_stage; }

    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::SHADER; }

protected:
    Shader(ShaderStage stage) : m_stage(stage) {};
    ShaderStage m_stage;
};


}