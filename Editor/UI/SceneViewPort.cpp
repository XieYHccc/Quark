#include "Editor/UI/SceneViewPort.h"
#include <Quark/Core/Application.h>
#include <Quark/UI/UI.h>
#include <Quark/Events/EventManager.h>

namespace quark {
SceneViewPort::SceneViewPort()
{
    m_Device = Application::Get().GetGraphicDevice();

    graphic::SamplerDesc samplerDesc;
    samplerDesc.minFilter = graphic::SamplerFilter::LINEAR;
    samplerDesc.magFliter = graphic::SamplerFilter::LINEAR;
    samplerDesc.addressModeU = graphic::SamplerAddressMode::REPEAT;
    samplerDesc.addressModeV = graphic::SamplerAddressMode::REPEAT;
    samplerDesc.addressModeW = graphic::SamplerAddressMode::REPEAT;
    m_Sampler = m_Device->CreateSampler(samplerDesc);

    m_Panelsize = { 0, 0 };
}

void SceneViewPort::SetColorAttachment(const graphic::Image* colorAttachment)
{
    auto find = m_IdMap.find(colorAttachment);

    if (find != m_IdMap.end()) {
        m_CurrentId = find->second;
    }
    else {
        m_CurrentId = UI::Get()->CreateTextureId(*colorAttachment, *m_Sampler);
        m_IdMap.insert({colorAttachment, m_CurrentId});
    }
}

void SceneViewPort::Render()
{
    CORE_DEBUG_ASSERT(m_CurrentId)

    ImGui::Begin("Scene");
    if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
        EventManager::Instance().TriggerEvent(SceneViewPortTouchedEvent());
    }

	m_Panelsize = ImGui::GetContentRegionAvail();
	ImGui::Image(m_CurrentId, m_Panelsize);
	
	ImGui::End();
}

}