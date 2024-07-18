#pragma once
#include "Graphic/Common.h"

namespace graphic {
    
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
    ShaderStage GetStage() const {return stage_; }
protected:
    Shader(ShaderStage stage) : stage_(stage) {};
    ShaderStage stage_;
};


}