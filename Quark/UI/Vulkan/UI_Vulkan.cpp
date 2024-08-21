#include "Quark/qkpch.h"
#include "Quark/UI/Vulkan/UI_Vulkan.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <backends/imgui_impl_vulkan.h>
#include "Quark/Core/Window.h"
#include "Quark/Graphic/Vulkan/CommandList_Vulkan.h"
#include "Quark/Events/EventManager.h"

namespace quark {

void UI_Vulkan::Init(graphic::Device* device, const UiInitSpecs& specs)
{
    CORE_DEBUG_ASSERT(device)

    device_ = static_cast<graphic::Device_Vulkan*>(device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= specs.flags & UI_INIT_FLAG_DOCKING? ImGuiConfigFlags_DockingEnable : 0;
    io.ConfigFlags |= specs.flags & UI_INIT_FLAG_VIEWPORTS? ImGuiConfigFlags_ViewportsEnable : 0;
	io.ConfigViewportsNoAutoMerge = false;
	io.ConfigViewportsNoTaskBarIcon = true;

    io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Bold.ttf", 18.f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Regular.ttf", 18.f);

    // Style
    {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

    	auto& colors = style.Colors;

        // Windows
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    }

    // Descripto pool
    {
        VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 100;
        pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(device_->vkDevice, &pool_info, nullptr, &descriptorPool_))
    }

    // Vulkan backend
    {
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Window::Instance()->GetNativeWindow(), true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = device_->context->instance;
        init_info.PhysicalDevice = device_->context->physicalDevice;
        init_info.Device = device_->vkDevice;
        init_info.Queue = device_->context->graphicQueue;
        init_info.QueueFamily = device_->context->graphicQueueIndex;
        init_info.DescriptorPool = descriptorPool_;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.UseDynamicRendering = true;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // TODO: MSAA
        init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        colorFormat_ = graphic::ConvertDataFormat(device_->GetSwapChainImageFormat());
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat_;
        ImGui_ImplVulkan_Init(&init_info);
    }

    CORE_LOGI("UI system initialized in vulkan backend")
}

void UI_Vulkan::Finalize()
{
    vkDeviceWaitIdle(device_->vkDevice);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(device_->vkDevice, descriptorPool_, nullptr);
    
}

void UI_Vulkan::BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    float minWinSizeX = style.WindowMinSize.x;
    style.WindowMinSize.x = 360.0f;
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
    style.WindowMinSize.x = minWinSizeX;
}

void UI_Vulkan::EndFrame()
{
    // Make imgui calculate internal draw structures
    ImGui::Render();
}

void UI_Vulkan::Render(graphic::CommandList* cmd)
{
    auto& internal = graphic::ToInternal(cmd);

    auto* data = ImGui::GetDrawData();
    if (data) {
        ImGui_ImplVulkan_RenderDrawData(data, internal.GetHandle());
    }

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

ImTextureID UI_Vulkan::CreateTextureId(const Ref<Texture>& texture)
{
    VkSampler samp = graphic::ToInternal(texture->sampler.get()).GetHandle();
    VkImageView view = graphic::ToInternal(texture->image.get()).GetView();

    return (ImTextureID)ImGui_ImplVulkan_AddTexture(samp, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}


ImTextureID UI_Vulkan::CreateTextureId(const graphic::Image& image, const graphic::Sampler& sampler)
{
    VkSampler samp = graphic::ToInternal(&sampler).GetHandle();
    VkImageView view = graphic::ToInternal(&image).GetView();

    return (ImTextureID)ImGui_ImplVulkan_AddTexture(samp, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
}