#pragma once
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Graphic/Common.h"

namespace quark {

// Program is a higher level abstraction over the Shader and PipeLine
// This class is responsible for managing the pipelines that created with same shaders but different other states
// In vulkan, this can be described as a pipeline layout with different pipeline objects

class Program {
public:
    Program(Ref<graphic::Shader> vertex, Ref<graphic::Shader> fragment);

    Ref<graphic::PipeLine> GetOrCreatePipeLine();

private:
    Ref<graphic::Shader> m_Shaders[util::ecast(graphic::ShaderStage::MAX_ENUM)];

    std::unordered_map<uint64_t, Ref<graphic::PipeLine>> m_PipeLines;
};
}