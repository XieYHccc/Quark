#pragma once
#include "Quark/UI/UI.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#include "Quark/Events/MouseEvent.h"

namespace quark {

class UI_Vulkan final : public UI {
public:
    UI_Vulkan() = default;
    ~UI_Vulkan() = default;

    void Init(graphic::Device* device, const UiInitSpecs& specs) override;
    void Finalize() override;

    void BeginFrame() override;
    void EndFrame() override;
    void Render(graphic::CommandList* cmd) override;

    ImTextureID CreateTextureId(const graphic::Image& image, const graphic::Sampler& sampler) override;
private:
    
    graphic::Device_Vulkan* device_;
    VkDescriptorPool descriptorPool_;
    VkFormat colorFormat_;
    // VkPipelineLayout pipelineLayout_;
    // VkPipeline pipeline_;

};

}