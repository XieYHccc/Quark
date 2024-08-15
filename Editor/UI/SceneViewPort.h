#pragma once
#include "Editor/UI/Common.h"
#include <Quark/Graphic/Device.h>
#include <Quark/Events/Event.h>
#include <imgui.h>

namespace quark {

class SceneViewPortTouchedEvent : public Event {
public:
    SceneViewPortTouchedEvent() = default;
    EVENT_TYPE("SceneViewPortTouchedEvent")

    virtual std::string ToString() const override
    {
        return "SceneViewPortTouchedEvent";
    }

};

class SceneViewPort : public UIWindowBase
{
public:
    SceneViewPort();

    void Render() override;
    void SetColorAttachment(const graphic::Image* colorAttachment);

private:
    Ref<graphic::Sampler> m_Sampler;
    ImTextureID m_CurrentId;
    std::unordered_map<const graphic::Image*, ImTextureID> m_IdMap;
    graphic::Device* m_Device;
    ImVec2 m_Panelsize;
};

}