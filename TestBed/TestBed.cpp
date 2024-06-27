#include "TestBed/TestBed.h"

TestBed::TestBed(const std::string& title, const std::string& root, int width, int height)
    : Application(title, root, width, height)
{
    using namespace graphic;
    auto graphic_device = Application::Instance().GetGraphicDevice();

    // Create shader
    vert_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/mesh.vert.spv");
    frag_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/mesh.frag.spv");
    
    // Create pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_blend(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
    pipe_desc.depthAttachmentFormat = DataFormat::D32_SFLOAT;
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

    scene = CreateScope<scene::Scene>("Testbed");
    
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
   
}   

void TestBed::Render(f32 deltaTime)
{
    
    auto graphic_device = Application::Instance().GetGraphicDevice();

    if (graphic_device->BeiginFrame(deltaTime)) {

        graphic::BufferDesc desc = {
            .domain = graphic::BufferMemoryDomain::GPU,
            .size = 16,
            .type = graphic::BufferType::UNIFORM_BUFFER
        };

        static const uint32_t checkerboard[] = {
            0u, ~0u, 0u, ~0u,
            ~0u, 0u, ~0u, 0u,
            0u, ~0u, 0u, ~0u,
            ~0u, 0u, ~0u, 0u,
        };
        auto buffer = graphic_device->CreateBuffer(desc, checkerboard);

        graphic::ImageDesc image_desc = {
            .format = graphic::DataFormat::R8G8B8A8_UNORM,
            .height = 4,
            .width = 4,
            .depth = 1,
            .mipLevels = 1,
            .initialLayout = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            .type = graphic::ImageType::TYPE_2D,
            .usageBits = graphic::IMAGE_USAGE_SAMPLING_BIT | graphic::IMAGE_USAGE_CAN_COPY_TO_BIT
        };
        graphic::ImageInitData image_data = {
            .data = checkerboard,
            .image_width = 4,
            .image_heigt = 4
        };

        auto image = graphic_device->CreateImage(image_desc, &image_data);

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

        // 3. Begin a render pass
        graphic::RenderPassInfo render_pass_info;
        render_pass_info.numColorAttachments = 1;
        render_pass_info.colorAttachments[0] = swap_chain_image;
        render_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
        render_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
        render_pass_info.clearColors[0].color[0] = 0.f;
        render_pass_info.clearColors[0].color[1] = 0.f;
        render_pass_info.clearColors[0].color[2] = 0.3f;
        render_pass_info.clearColors[0].color[3] = 0.f;
        cmd->BeginRenderPass(render_pass_info);

        cmd->BindPipeLine(*graphic_pipeline);
        //....
        
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
        graphic_device->EndFrame(deltaTime);
    }
}