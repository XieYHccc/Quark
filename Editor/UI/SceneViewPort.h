#pragma once
#include "Editor/UI/Common.h"
#include <Quark/Graphic/Device.h>
#include <Quark/Events/Event.h>
#include <imgui.h>

namespace editor::ui {

class SceneViewPortTouchedEvent : public Event {
public:
    SceneViewPortTouchedEvent() = default;
    EVENT_TYPE("SceneViewPortTouchedEvent")
};

class SceneViewPort : public UIWindowBase
{
public:
    SceneViewPort() = default;

    void Init() override;
    void Render() override;
    void SetColorAttachment(const graphic::Image* colorAttachment);

private:
    Ref<graphic::Sampler> sampler_;
    ImTextureID currentId_;
    std::unordered_map<const graphic::Image*, ImTextureID> idMap_;
    graphic::Device* device_;
    ImVec2 panelsize_;
};

}