#include "TestBed/TestBed.h"
#include <glm/gtx/quaternion.hpp>
#include <Engine/Asset/GLTFLoader.h>
#include <Engine/Scene/Components/TransformCmpt.h>
#include <Engine/Scene/Components/CameraCmpt.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Input.h>

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
    auto* cam = scene->GetCamera();
    auto* camTrans = (cam->GetEntity()->GetComponent<scene::TransformCmpt>());
    float moveSpeed = 30;
    float mouseSensitivity = 0.3;

    // 1. process mouse inputs
    MousePosition pos = Input::GetMousePosition();

    if (Input::first_mouse) {
        Input::last_position = pos;
        Input::first_mouse = false;
    }

    float xoffset = pos.x_pos - Input::last_position.x_pos;
    float yoffset = pos.y_pos - Input::last_position.y_pos;

    pitch -= (glm::radians(yoffset) * mouseSensitivity);
    yaw -= (glm::radians(xoffset) * mouseSensitivity);
    // make sure that when pitch is out of bounds, screen doesn't get flipped
    pitch = std::clamp(pitch, -1.5f, 1.5f);

    Input::last_position = pos;

    // 2. process keyboard inputs
    glm::vec3 move {0.f};
    if (Input::IsKeyPressed(W))
        move.z = -1;
    if (Input::IsKeyPressed(S))
        move.z = 1;
    if (Input::IsKeyPressed(A))
        move.x = -1;
    if (Input::IsKeyPressed(D))
        move.x = 1;
    move = move * moveSpeed * 0.01f;

    // 3.update camera's transform
    camTrans->SetEuler(glm::vec3(pitch, yaw, 0));
    glm::mat4 rotationMatrix = glm::toMat4(camTrans->GetQuat());
    camTrans->SetPosition(camTrans->GetPosition() + glm::vec3(rotationMatrix * glm::vec4(move, 0.f)));
    scene->Update();
}   

void TestBed::Render(f32 deltaTime)
{
    auto graphic_device = Application::Instance().GetGraphicDevice();
    
    auto start = std::chrono::system_clock::now();

    if (graphic_device->BeiginFrame(deltaTime)) {
        // 1. Begin a graphic command list
        auto cmd = graphic_device->BeginCommandList();

        // 2. Query swapchain image and add layout transition barrier
        auto swap_chain_image = graphic_device->GetSwapChainImage();
        
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
        render_pass_info.depthAttatchment = depth_image;
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

        auto geometry_start = std::chrono::system_clock::now();
        scene_renderer->Render(cmd);
        auto geometry_end = std::chrono::system_clock::now();
        auto geometry_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(geometry_end - geometry_start);
        CORE_LOGI("Geometry time : {} ms", geometry_elapsed.count() / 1000.f)

        cmd->EndRenderPass();

        // 4. Transit swapchain image to present layout for presenting
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


        // 5. Submit command list
        graphic_device->SubmitCommandList(cmd);

        // End this frame, submit Command list and pressent to screen
        auto startFrame = std::chrono::system_clock::now();
        graphic_device->EndFrame(deltaTime);
        auto endFrame = std::chrono::system_clock::now();
        auto elapsedFrame = std::chrono::duration_cast<std::chrono::microseconds>(endFrame - startFrame);
        CORE_LOGI("End Frame time : {} ms", elapsedFrame.count() / 1000.f)
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    CORE_LOGI("Darw time : {} ms", elapsed.count() / 1000.f)
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

    // Default position
    auto* transform_cmpt = cam_node->GetEntity()->GetComponent<scene::TransformCmpt>();
    transform_cmpt->SetPosition(glm::vec3(0, 0, 10));

    scene->SetCamera(cam_node);
    
}