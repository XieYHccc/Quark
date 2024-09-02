#pragma once
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Graphic/Common.h"

namespace quark {

// This class contains all pipelines that created with same shaders but different other states
class ShaderProgram {
public:
    ShaderProgram(Ref<graphic::Shader> vertex, Ref<graphic::Shader> fragment);
private:
    Ref<graphic::Shader> m_Shaders[util::ecast(graphic::ShaderStage::MAX_ENUM)];

    std::unordered_map<uint64_t, Ref<graphic::PipeLine>> m_PipeLines;
};
}