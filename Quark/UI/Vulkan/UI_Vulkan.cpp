#include "Quark/qkpch.h"
#include "Quark/UI/Vulkan/UI_Vulkan.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <backends/imgui_impl_vulkan.h>

#include "Quark/Core/Window.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Graphic/Vulkan/CommandList_Vulkan.h"

namespace quark {

void UI_Vulkan::Init(graphic::Device* device, const UiInitSpecs& specs)
{
    CORE_DEBUG_ASSERT(device)

    m_device = static_cast<graphic::Device_Vulkan*>(device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= specs.flags & UI_INIT_FLAG_DOCKING? ImGuiConfigFlags_DockingEnable : 0;
    io.ConfigFlags |= specs.flags & UI_INIT_FLAG_VIEWPORTS? ImGuiConfigFlags_ViewportsEnable : 0;
	io.ConfigViewportsNoAutoMerge = false;
	io.ConfigViewportsNoTaskBarIcon = true;

    io.Fonts->AddFontFromFileTTF("BuiltInResources/Fonts/OpenSans/OpenSans-Bold.ttf", 18.f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF("BuiltInResources/Fonts/OpenSans/OpenSans-Regular.ttf", 18.f);

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
        VK_CHECK(vkCreateDescriptorPool(m_device->vkDevice, &pool_info, nullptr, &m_descriptorPool))
    }

    // Vulkan backend
    {
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Window::Instance()->GetNativeWindow(), true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_device->vkContext->instance;
        init_info.PhysicalDevice = m_device->vkContext->physicalDevice;
        init_info.Device = m_device->vkDevice;
        init_info.Queue = m_device->vkContext->graphicQueue;
        init_info.QueueFamily = m_device->vkContext->graphicQueueIndex;
        init_info.DescriptorPool = m_descriptorPool;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.UseDynamicRendering = true;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // TODO: MSAA
        init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        
        // This format pointer must be cached due to some weird state tracking of imgui
        m_colorFormat = graphic::ConvertDataFormat(m_device->GetSwapChainImageFormat());
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_colorFormat;
        ImGui_ImplVulkan_Init(&init_info);
    }

    CORE_LOGI("[UI]: Initialized in vulkan backend")
}

void UI_Vulkan::Finalize()
{
    vkDeviceWaitIdle(m_device->vkDevice);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_device->vkDevice, m_descriptorPool, nullptr);
    
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

ImTextureID UI_Vulkan::GetOrCreateTextureId(const Ref<Texture>& texture)
{
    util::Hasher h;
    h.pointer(texture->image.get());
    h.pointer(texture->sampler.get());
    util::Hash hash = h.get();

    auto it = m_textureIdMap.find(hash);
    if (it != m_textureIdMap.end())
    {
        return it->second;
    }
    else
    {
        VkSampler samp = graphic::ToInternal(texture->sampler.get()).GetHandle();
        VkImageView view = graphic::ToInternal(texture->image.get()).GetView();

        ImTextureID newId = (ImTextureID)ImGui_ImplVulkan_AddTexture(samp, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_textureIdMap[hash] = newId;

        return newId;
    }
}


ImTextureID UI_Vulkan::GetOrCreateTextureId(const Ref<graphic::Image>& image, const Ref<graphic::Sampler>& sampler)
{
    util::Hasher h;
    h.pointer(image.get());
    h.pointer(sampler.get());
    util::Hash hash = h.get();

    auto it = m_textureIdMap.find(hash);
    if (it != m_textureIdMap.end())
    {
        return it->second;
    }
    else
    {
        VkSampler samp = graphic::ToInternal(sampler.get()).GetHandle();
        VkImageView view = graphic::ToInternal(image.get()).GetView();

        ImTextureID newId = (ImTextureID)ImGui_ImplVulkan_AddTexture(samp, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_textureIdMap[hash] = newId;

        return newId;
    }
}
}