#pragma once
#include <imgui.h>
#include "Util/Singleton.h"
#include "Graphic/Device.h"

enum UiInitFlagBit{
    UI_INIT_FLAG_DOCKING = 1 << 0,
    UI_INIT_FLAG_VIEWPORTS = 1 << 1,
};  

struct UiInitSpecs {
    std::uint32_t flags;
};

class UI : public util::MakeSingletonPtr<UI>{
public:
    
    UI() = default;
    virtual ~UI() = default;

    virtual void Init(graphic::Device* device, const UiInitSpecs& sepcs) = 0;
    virtual void Finalize() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Render(graphic::CommandList* cmd) = 0;

    virtual ImTextureID CreateTextureId(const graphic::Image& image, const graphic::Sampler& sampler) = 0;

};

template <>
template <>
UI* util::MakeSingletonPtr<UI>::CreateSingleton();