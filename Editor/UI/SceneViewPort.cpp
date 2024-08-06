#include "Editor/UI/SceneViewPort.h"
#include <Quark/Core/Application.h>
#include <Quark/UI/UI.h>
#include <Quark/Events/EventManager.h>

namespace editor::ui {

void SceneViewPort::Init()
{
    device_ = Application::Instance().GetGraphicDevice();

    graphic::SamplerDesc samplerDesc;
    samplerDesc.minFilter = graphic::SamplerFilter::LINEAR;
    samplerDesc.magFliter = graphic::SamplerFilter::LINEAR;
    samplerDesc.addressModeU = graphic::SamplerAddressMode::REPEAT;
    samplerDesc.addressModeV = graphic::SamplerAddressMode::REPEAT;
    samplerDesc.addressModeW = graphic::SamplerAddressMode::REPEAT;
    sampler_ = device_->CreateSampler(samplerDesc);

    panelsize_ = {0, 0};
}

void SceneViewPort::SetColorAttachment(const graphic::Image* colorAttachment)
{
    auto find = idMap_.find(colorAttachment);

    if (find != idMap_.end()) {
        currentId_ = find->second;
    }
    else {
        currentId_ = UI::Singleton()->CreateTextureId(*colorAttachment, *sampler_);
        idMap_.insert({colorAttachment, currentId_});
    }
}

void SceneViewPort::Render()
{
    CORE_DEBUG_ASSERT(currentId_)

    ImGui::Begin("Scene");
    if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
        EventManager::Instance().TriggerEvent(SceneViewPortTouchedEvent());
    }

	panelsize_ = ImGui::GetContentRegionAvail();
	ImGui::Image(currentId_, panelsize_);
	
	ImGui::End();
}

}