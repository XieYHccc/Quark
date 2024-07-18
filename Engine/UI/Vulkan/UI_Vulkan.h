#pragma once
#include "UI/UI.h"
#include "Graphic/Vulkan/Device_Vulkan.h"

class UI_Vulkan : public UI {
public:
    UI_Vulkan() = default;
    virtual ~UI_Vulkan() = default;

    virtual void Init(graphic::Device* device) override;
    virtual void Finalize() override;

    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void Render(graphic::CommandList* cmd) override;

private:
    graphic::Device_Vulkan* device_;
    VkDescriptorPool descriptorPool_;
    // VkPipelineLayout pipelineLayout_;
    // VkPipeline pipeline_;

};