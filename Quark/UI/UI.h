#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/RHI/Common.h"

#include <imgui.h>

namespace quark {

enum UiInitFlagBit
{
    UI_INIT_FLAG_DOCKING = 1 << 0,
    UI_INIT_FLAG_VIEWPORTS = 1 << 1,
};  

struct UiSpecification 
{
    uint32_t flags;
};

class UI : public util::MakeSingletonPtr<UI>{
public:
    
    UI() = default;
    virtual ~UI() = default;

    virtual void Init(Ref<rhi::Device> device, const UiSpecification& sepcs) = 0;
    virtual void Finalize() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void OnRender(rhi::CommandList* cmd) = 0;

    virtual ImTextureID GetOrCreateTextureId(const Ref<rhi::Image>& image, const Ref<rhi::Sampler>& sampler) = 0;
};

template <>
template <>
UI* util::MakeSingletonPtr<UI>::CreateSingleton();

}