#include <chrono>
#include <array>

#include <Quark/Core/Application.h>
#include <Quark/Renderer/GpuResourceManager.h>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>

namespace quark {
class TextureExample : public Application {
public:
    // Vertex layout used in this example
    struct Vertex {
        float position[3];
        float texCoords[2];
    };

    struct uniformBufferData {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    } uniform_buffer_data;

    // Vertex buffer
    Ref<graphic::Buffer> vert_buffer;

    // Index buffer
    Ref<graphic::Buffer> index_buffer;

    // texture image and sampler
    Ref<graphic::Image> texture_image;
    Ref<graphic::Sampler> linear_sampler;
    graphic::DataFormat texture_format = graphic::DataFormat::R8G8B8A8_UNORM;

    // Render Pass Info
    graphic::RenderPassInfo2 render_pass_info;

    // Shaders and pipeline
    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    void CreateVertexBuffer()
    {
        using namespace quark::graphic;

        // Setup vertices data
        std::vector<Vertex> vertexBufferData{
            { {  -1.0f,  1.0f, -3.0f }, { -1.0f, 1.0f} },
            { { 1.0f,  1.0f, -3.0f }, { 1.0f, 1.0f } },
            { {  -1.f, -1.0f, -3.0f }, { -1.f, -1.f } },
            { {1.f, -1.f, -3.f}, { 1.f, -1.f} }
        };

        // Buffer create description
        BufferDesc buffer_desc;
        buffer_desc.domain = BufferMemoryDomain::GPU;
        buffer_desc.size = sizeof(Vertex) * vertexBufferData.size();
        buffer_desc.usageBits = BUFFER_USAGE_VERTEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_TO_BIT;

        // Create vertex buffer
        vert_buffer = m_GraphicDevice->CreateBuffer(buffer_desc, vertexBufferData.data());
    }

    void CreateIndexBuffer()
    {
        using namespace quark::graphic;

        // Setup indices data
        std::vector<uint32_t> indexBuffer{ 0, 2, 3, 0, 3, 1 };

        // Buffer create description
        graphic::BufferDesc buffer_desc;
        buffer_desc.domain = BufferMemoryDomain::GPU;
        buffer_desc.size = indexBuffer.size() * sizeof(uint32_t);
        buffer_desc.usageBits = BUFFER_USAGE_INDEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_TO_BIT;

        // Create index buffer
        index_buffer = m_GraphicDevice->CreateBuffer(buffer_desc, indexBuffer.data());
    }

    void CreateTextureImage()
    {
        using namespace quark::graphic;
        // prepare data for 16x16 checkerboard texture
        constexpr uint32_t black = 0x000000FF;
        constexpr uint32_t magenta = 0xFF00FFFF;
        std::array<uint32_t, 32 * 32 > pixels;
        for (int x = 0; x < 32; x++) {
            for (int y = 0; y < 32; y++) {
                pixels[y * 32 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        // Image create info
        ImageDesc texture_desc;
        texture_desc.type = ImageType::TYPE_2D;
        texture_desc.width = 32;
        texture_desc.height = 32;
        texture_desc.depth = 1;
        texture_desc.format = texture_format;
        texture_desc.arraySize = 1;
        texture_desc.mipLevels = 1;
        texture_desc.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        texture_desc.usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT;

        std::vector<ImageInitData> init_data;
        init_data.push_back({ pixels.data(), 32, 32 });

        // Create gpu resource
        texture_image = m_GraphicDevice->CreateImage(texture_desc, init_data.data());
    }

    void CreateSampler()
    {
        // Sampler create description
        graphic::SamplerDesc desc;
        desc.addressModeU = graphic::SamplerAddressMode::REPEAT;
        desc.addressModeV = graphic::SamplerAddressMode::REPEAT;
        desc.addressModeW = graphic::SamplerAddressMode::REPEAT;
        desc.minFilter = graphic::SamplerFilter::LINEAR;
        desc.magFliter = graphic::SamplerFilter::LINEAR;
        desc.enableAnisotropy = false;
        linear_sampler = m_GraphicDevice->CreateSampler(desc);
    }

    void CreateRenderPassInfo()
    {
        render_pass_info = {};
        render_pass_info.numColorAttachments = 1;
        render_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetPresentImageFormat();
    }

    void CreateGraphicPipeline()
    {
        using namespace quark::graphic;

        // Create shader
        vert_shader = m_GraphicDevice->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
            "BuiltInResources/Shaders/Spirv/texture.vert.spv");
        frag_shader = m_GraphicDevice->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
            "BuiltInResources/Shaders/Spirv/texture.frag.spv");

        // Graphic pipeline create info
        GraphicPipeLineDesc pipe_desc;
        pipe_desc.vertShader = vert_shader;
        pipe_desc.fragShader = frag_shader;
        pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
        pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
        pipe_desc.renderPassInfo2 = render_pass_info;

        // Depth-stencil state
        pipe_desc.depthStencilState = {
            .enableDepthTest = false,
            .enableDepthWrite = false,
            .depthCompareOp = CompareOperation::LESS_OR_EQUAL
        };

        // rasterization state
        pipe_desc.rasterState.cullMode = CullMode::NONE;
        pipe_desc.rasterState.polygonMode = PolygonMode::Fill;
        pipe_desc.rasterState.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;

        // Vertex Input Layout
        VertexInputLayout::VertexBindInfo& vert_bind_info = pipe_desc.vertexInputLayout.vertexBindInfos.emplace_back();
        vert_bind_info.binding = 0;
        vert_bind_info.stride = sizeof(Vertex);
        vert_bind_info.inputRate = VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX;

        // Vertex attributes info
        auto& pos_attrib = pipe_desc.vertexInputLayout.vertexAttribInfos.emplace_back();
        pos_attrib.binding = 0;
        pos_attrib.format = VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3;
        pos_attrib.location = 0;
        pos_attrib.offset = offsetof(Vertex, position);

        auto& uv_attrib = pipe_desc.vertexInputLayout.vertexAttribInfos.emplace_back();
        uv_attrib.binding = 0;
        uv_attrib.format = VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC2;
        uv_attrib.location = 1;
        uv_attrib.offset = offsetof(Vertex, texCoords);

        graphic_pipeline = m_GraphicDevice->CreateGraphicPipeLine(pipe_desc);
    }


    TextureExample(const ApplicationSpecification& spec)
        :Application(spec)
    {
        CreateRenderPassInfo();
        CreateGraphicPipeline();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateTextureImage();
        CreateSampler();
    }

    ~TextureExample() {};

    void OnUpdate(TimeStep deltaTime) override
    {

    }

    void OnRender(TimeStep deltaTime) override
    {
        using namespace quark::graphic;

        // auto start = std::chrono::system_clock::now();

        auto graphic_device = Application::Get().GetGraphicDevice();

        if (graphic_device->BeiginFrame(deltaTime)) {

            // 1. Begin a graphic command list
            auto cmd = graphic_device->BeginCommandList();

            // 2. Query swapchain image and add layout transition barrier
            auto swap_chain_image = graphic_device->GetPresentImage();

            graphic::PipelineImageBarrier swapchain_image_barrier;
            swapchain_image_barrier.image = swap_chain_image;
            swapchain_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            swapchain_image_barrier.srcMemoryAccessBits = 0;
            swapchain_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            swapchain_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            swapchain_image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
            swapchain_image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

            // 3. Begin a render pass
            FrameBufferInfo frameBufferInfo;
            frameBufferInfo.colorAttachments[0] = swap_chain_image;
            frameBufferInfo.clearColors[0] = { 0.f, 0.f, 0.4f, 1.f };
            frameBufferInfo.colorAttatchemtsLoadOp[0] = FrameBufferInfo::AttachmentLoadOp::CLEAR;
            frameBufferInfo.colorAttatchemtsStoreOp[0] = FrameBufferInfo::AttachmentStoreOp::STORE;
            cmd->BeginRenderPass(render_pass_info, frameBufferInfo);

            // 4. Bind pipeline and set viewport and scissor
            cmd->BindPipeLine(*graphic_pipeline);

            cmd->BindPipeLine(*graphic_pipeline);

            u32 drawWidth = swap_chain_image->GetDesc().width;
            u32 drawHeight = swap_chain_image->GetDesc().height;
            Viewport vp;
            vp.x = 0;
            vp.y = 0;
            vp.width = (float)drawWidth;
            vp.height = (float)drawHeight;
            vp.minDepth = 0;
            vp.maxDepth = 1;
            Scissor scissor;
            scissor.extent.width = drawWidth;
            scissor.extent.height = drawHeight;
            scissor.offset.x = 0;
            scissor.offset.y = 0;

            cmd->SetViewPort(vp);
            cmd->SetScissor(scissor);

            // 5.Create uniform buffer
            BufferDesc uniform_buffer_desc;
            uniform_buffer_desc.domain = BufferMemoryDomain::CPU;
            uniform_buffer_desc.size = sizeof(uniformBufferData);
            uniform_buffer_desc.usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            Ref<Buffer> uniform_buffer = graphic_device->CreateBuffer(uniform_buffer_desc);

            // Fill uniform buffer
            uniformBufferData& mapped_data = *(uniformBufferData*)uniform_buffer->GetMappedDataPtr();
            mapped_data.modelMatrix = glm::mat4(1.f);
            mapped_data.viewMatrix = glm::mat4(1.f);
            float aspect = GetWindow()->GetRatio();
            mapped_data.projectionMatrix = glm::perspective(glm::radians(45.f), aspect, 0.1f, 100.f);
            mapped_data.projectionMatrix[1] *= -1; // screen space's Y-axis is opposite of world space's Y-axis
            cmd->BindUniformBuffer(0, 0, *uniform_buffer, 0, uniform_buffer->GetDesc().size);

            // 6. Bind uniform buffer, texture, and sampler
            cmd->BindUniformBuffer(0, 0, *uniform_buffer, 0, uniform_buffer->GetDesc().size);
            cmd->BindImage(0, 1, *GpuResourceManager::Get().image_checkboard, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(0, 1, *linear_sampler);

            // 7.Bind vertex buffer and index buffer
            cmd->BindVertexBuffer(0, *vert_buffer, 0);
            cmd->BindIndexBuffer(*index_buffer, 0, IndexBufferFormat::UINT32);

            // 8. Draw call
            cmd->DrawIndexed(index_buffer->GetDesc().size / sizeof(uint32_t), 1, 0, 0, 0);
            cmd->EndRenderPass();

            // 9. Transit swapchain image to present layout for presenting
            graphic::PipelineImageBarrier present_barrier;
            present_barrier.image = swap_chain_image;
            present_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            present_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            present_barrier.dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            present_barrier.dstMemoryAccessBits = 0;
            present_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            present_barrier.layoutAfter = graphic::ImageLayout::PRESENT;
            cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);

            // 10. Submit command list
            graphic_device->SubmitCommandList(cmd);

            // End this frame, submit Command list and pressent to screen
            graphic_device->EndFrame(deltaTime);
        }

        // auto end = std::chrono::system_clock::now();
        // auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        // CORE_LOGD("Darw time : {}", elapsed.count() / 1000000000.f)
    }

};

Application* CreateApplication()
{
    ApplicationSpecification spec;
    spec.title = "Texture";
    spec.height = 800;
    spec.width = 1200;

    return new TextureExample(spec);

}

}