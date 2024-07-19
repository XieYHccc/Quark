#pragma once
#include "UI/UI.h"
#include "Graphic/Vulkan/Device_Vulkan.h"

class UI_Vulkan final : public UI {
public:
    UI_Vulkan() = default;
    ~UI_Vulkan() = default;

    void Init(graphic::Device* device) override;
    void Finalize() override;

    void BeginFrame() override;
    void EndFrame() override;
    void Render(graphic::CommandList* cmd) override;

    bool BeginBlock(const char* name, WindowFlags flags = 0) override;
    void EndBlock() override;

    void Text(const char* formatstr, ...) override;
private:
    graphic::Device_Vulkan* device_;
    VkDescriptorPool descriptorPool_;
    // VkPipelineLayout pipelineLayout_;
    // VkPipeline pipeline_;

};