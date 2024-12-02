#pragma once
#include "Quark/UI/UI.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Events/MouseEvent.h"

namespace quark {

class UI_Vulkan final : public UI {
public:
    UI_Vulkan() = default;
    ~UI_Vulkan() = default;

    void Init(rhi::Device* device, const UiSpecification& specs) override;
    void Finalize() override;

    void BeginFrame() override;
    void EndFrame() override;
    void OnRender(rhi::CommandList* cmd) override;

    ImTextureID GetOrCreateTextureId(const Ref<Texture>& texture) override;
    ImTextureID GetOrCreateTextureId(const Ref<rhi::Image>& image, const Ref<rhi::Sampler>& sampler) override;

private:
    rhi::Device_Vulkan* m_device;

    VkDescriptorPool m_descriptorPool;
    VkFormat m_colorFormat;

    std::unordered_map<uint64_t, ImTextureID> m_textureIdMap;
    

};

}