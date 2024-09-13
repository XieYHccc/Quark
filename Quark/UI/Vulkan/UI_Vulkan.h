#pragma once
#include "Quark/UI/UI.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#include "Quark/Events/MouseEvent.h"

namespace quark {

class UI_Vulkan final : public UI {
public:
    UI_Vulkan() = default;
    ~UI_Vulkan() = default;

    void Init(graphic::Device* device, const UiSpecification& specs) override;
    void Finalize() override;

    void BeginFrame() override;
    void EndFrame() override;
    void Render(graphic::CommandList* cmd) override;

    ImTextureID GetOrCreateTextureId(const Ref<Texture>& texture) override;
    ImTextureID GetOrCreateTextureId(const Ref<graphic::Image>& image, const Ref<graphic::Sampler>& sampler) override;

private:
    graphic::Device_Vulkan* m_device;

    VkDescriptorPool m_descriptorPool;
    VkFormat m_colorFormat;

    std::unordered_map<uint64_t, ImTextureID> m_textureIdMap;
    

};

}