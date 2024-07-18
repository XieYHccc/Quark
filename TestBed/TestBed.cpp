#include "TestBed/TestBed.h"
#include "Scene/Components/MoveControlCmpt.h"
#include <glm/gtx/quaternion.hpp>
#include <Quark/Asset/GLTFLoader.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Core/Window.h>
#include <Quark/UI/UI.h>
#include <imgui.h>

TestBed::TestBed(const std::string& title, const std::string& root, int width, int height)
    : Application(title, root, width, height)
{
    CreatePipeline();
    CreateDepthImage();
    LoadAsset();
    SetUpCamera();
    scene_renderer->PrepareForRender();
}

Application* CreateApplication()
{
    return new TestBed("TestBed"," ", 1200, 800);
    
}

TestBed::~TestBed()
{
}

void TestBed::Update(f32 deltaTime)
{
    // Poll Inputs
    Input::Singleton()->Update();
    
    // Update camera movement
    auto* cam = scene->GetCamera();
    auto* camMoveCmpt = cam->GetEntity()->GetComponent<scene::MoveControlCmpt>();
    camMoveCmpt->Update(deltaTime);

    // Update scene
    scene->Update();
}   

void TestBed::Render(f32 deltaTime)
{
    // Prepare UI data
    UI::Singleton()->BeginFrame();
    // some imgui UI to test
    if (ImGui::Begin("background")) {
    
        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
        ImGui::End();
    }

    UI::Singleton()->EndFrame();

    // Fill command list
    auto graphic_device = Application::Instance().GetGraphicDevice();
    if (graphic_device->BeiginFrame(deltaTime)) {
        // 1. Begin a graphic command list
        auto cmd = graphic_device->BeginCommandList();

        // 2. Query swapchain image and add layout transition barrier
        auto swap_chain_image = graphic_device->GetPresentImage();
        
        graphic::PipelineImageBarrier swapchain_image_barrier{
            .image = swap_chain_image,
            .srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            .srcMemoryAccessBits = 0,
            .dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .layoutBefore = graphic::ImageLayout::UNDEFINED,
            .layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
        };
        cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

        // 3. Begin geometry render pass
        graphic::RenderPassInfo render_pass_info;
        render_pass_info.numColorAttachments = 1;
        render_pass_info.colorAttachments[0] = swap_chain_image;
        render_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
        render_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
        render_pass_info.clearColors[0].color[0] = 0.f;
        render_pass_info.clearColors[0].color[1] = 0.f;
        render_pass_info.clearColors[0].color[2] = 0.3f;
        render_pass_info.clearColors[0].color[3] = 1.f;
        render_pass_info.depthAttachment = depth_image.get();
        render_pass_info.depthAttachmentLoadOp = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
        render_pass_info.depthAttachmentStoreOp = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
        render_pass_info.ClearDepthStencil.depth_stencil = {1.f, 0};
        cmd->BeginRenderPass(render_pass_info);

        // 4. Set viewport and scissor
        u32 drawWidth = swap_chain_image->GetDesc().width;
        u32 drawHeight = swap_chain_image->GetDesc().height;
        cmd->SetViewPort(graphic::Viewport{.x = 0, .y = 0, .width = (float)drawWidth,
            .height = (float)drawHeight, .minDepth = 0, .maxDepth = 1});
        cmd->SetScissor(graphic::Scissor{.extent = {.width = drawWidth, .height = drawHeight},
            .offset = {.x = 0, .y = 0}});

        // 5. draw geometry
        cmd->BindPipeLine(*graphic_pipeline);

        auto geometry_start = m_Timer.ElapsedMillis();
        scene_renderer->Render(cmd);
        cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

        cmd->EndRenderPass();

        // 4. Draw UI
        graphic::RenderPassInfo ui_render_pass_info = {
            .numColorAttachments = 1,
            .colorAttachments[0] = swap_chain_image,
            .colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::LOAD,
            .colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE,
        };
        cmd->BeginRenderPass(ui_render_pass_info);
        UI::Singleton()->Render(cmd);
        cmd->EndRenderPass();

        // 5. Transit swapchain image to present layout for presenting
        graphic::PipelineImageBarrier present_barrier {
            .image = swap_chain_image,
            .srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstMemoryAccessBits = 0,
            .layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            .layoutAfter = graphic::ImageLayout::PRESENT
        };
        cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);


        // 6. Submit command list
        graphic_device->SubmitCommandList(cmd);

        // End this frame, submit Command list and pressent to screen
        graphic_device->EndFrame(deltaTime);
    }

}

void TestBed::LoadAsset()
{
    // load scene
    asset::GLTFLoader gltf_loader(m_GraphicDevice.get());
    scene = gltf_loader.LoadSceneFromFile("/Users/xieyhccc/develop/Quark/Assets/Gltf/structure.glb");

    scene_renderer = CreateScope<render::SceneRenderer>(m_GraphicDevice.get());
    scene_renderer->SetScene(scene.get());
}

void TestBed::CreatePipeline()
{
    using namespace graphic;
    auto graphic_device = Application::Instance().GetGraphicDevice();

    // Create shader
    vert_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/pbr.vert.spv");
    frag_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/pbr.frag.spv");
    
    // Create pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
    pipe_desc.depthAttachmentFormat = depth_format;
    pipe_desc.colorAttachmentFormats.push_back(graphic_device->GetSwapChainImageFormat());
    pipe_desc.depthStencilState = {
        .enableDepthTest = true,
        .enableDepthWrite = true,
        .depthCompareOp = CompareOperation::LESS_OR_EQUAL
    };
    pipe_desc.rasterState = {
        .cullMode = CullMode::NONE,
        .polygonMode = PolygonMode::Fill,
        .frontFaceType = FrontFaceType::COUNTER_CLOCKWISE
    };

    graphic_pipeline = graphic_device->CreateGraphicPipeLine(pipe_desc);
}

void TestBed::CreateDepthImage()
{
        using namespace graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Image create info
        ImageDesc depth_image_desc = {
            .type = ImageType::TYPE_2D,
            .width = graphic_device->GetResolutionWidth(),
            .height = graphic_device->GetResolutionHeight(),
            .depth = 1,
            .format = depth_format,
            .arraySize = 1,
            .mipLevels = 1,
            .initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        };

        // Create depth image
        depth_image = graphic_device->CreateImage(depth_image_desc);
}

void TestBed::SetUpCamera()
{   
    // Create camera node
    float aspect = (float)Window::Instance()->GetWidth() / Window::Instance()->GetHeight();
    auto* cam_node = scene->CreateNode("Main camera", nullptr);
    cam_node->GetEntity()->AddComponent<scene::CameraCmpt>(aspect, 60.f, 0.1f, 256);
    cam_node->GetEntity()->AddComponent<scene::MoveControlCmpt>(50, 0.3);

    // Default position
    auto* transform_cmpt = cam_node->GetEntity()->GetComponent<scene::TransformCmpt>();
    transform_cmpt->SetPosition(glm::vec3(0, 0, 10));

    scene->SetCamera(cam_node);
    
}