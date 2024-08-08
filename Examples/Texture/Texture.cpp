#include <chrono>
#include <Quark/Core/Application.h>
#include <Core/Window.h>

#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>

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
    graphic::RenderPassInfo render_pass_info;

    // Shaders and pipeline
    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    void CreateVertexBuffer()
    {
        using namespace quark::graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Setup vertices data
		std::vector<Vertex> vertexBufferData{
			{ {  -1.0f,  1.0f, -3.0f }, { -1.0f, 1.0f} },
			{ { 1.0f,  1.0f, -3.0f }, { 1.0f, 1.0f } },
			{ {  -1.f, -1.0f, -3.0f }, { -1.f, -1.f } },
            { {1.f, -1.f, -3.f}, { 1.f, -1.f} }
		};

        // Buffer create description
        graphic::BufferDesc buffer_desc = {
            .domain = BufferMemoryDomain::GPU,
            .size = sizeof(Vertex) * vertexBufferData.size(),
            .usageBits = BUFFER_USAGE_VERTEX_BUFFER_BIT | graphic::BUFFER_USAGE_TRANSFER_TO_BIT
        };

        // Create vertex buffer
        vert_buffer = graphic_device->CreateBuffer(buffer_desc, vertexBufferData.data());
    }

    void CreateIndexBuffer()
    {
        using namespace quark::graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Setup indices data
        std::vector<uint32_t> indexBuffer{ 0, 2, 3, 0, 3, 1 };

        // Buffer create description
        graphic::BufferDesc buffer_desc = {
            .domain = BufferMemoryDomain::GPU,
            .size = indexBuffer.size() * sizeof(uint32_t),
            .usageBits = BUFFER_USAGE_INDEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_TO_BIT
        };

        // Create index buffer
        index_buffer = graphic_device->CreateBuffer(buffer_desc, indexBuffer.data());
    }

    void CreateTextureImage()
    {
        using namespace quark::graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // prepare data for 16x16 checkerboard texture
        constexpr uint32_t black = __builtin_bswap32(0x000000FF);
        constexpr uint32_t magenta = __builtin_bswap32(0xFF00FFFF);
        std::array<uint32_t, 32 * 32 > pixels;
        for (int x = 0; x < 32; x++) {
            for (int y = 0; y < 32; y++) {
                pixels[y * 32 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        // Image create info
        ImageDesc texture_desc = {
            .type = ImageType::TYPE_2D,
            .width = 32,
            .height = 32,
            .depth = 1,
            .format = texture_format,
            .arraySize = 1,
            .mipLevels = 1,
            .initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            .usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT
        };
        std::vector<ImageInitData> init_data;
        init_data.push_back({pixels.data(), 32, 32});

        // Create gpu resource
        texture_image = graphic_device->CreateImage(texture_desc, init_data.data());
    }

    void CreateSampler()
    {
        auto* graphic_device = Application::Instance().GetGraphicDevice();

        // Sampler create description
        graphic::SamplerDesc desc = {
            .enableAnisotropy = false,
            .minFilter = graphic::SamplerFilter::LINEAR,
            .magFliter = graphic::SamplerFilter::LINEAR,
            .addressModeU = graphic::SamplerAddressMode::REPEAT,
            .addressModeV = graphic::SamplerAddressMode::REPEAT,
            .addressModeW = graphic::SamplerAddressMode::REPEAT
        };

        linear_sampler = graphic_device->CreateSampler(desc);
    }

    void SetUpRenderPass()
    {
        render_pass_info = {};
        render_pass_info.numColorAttachments = 1;
        render_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
        render_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
        render_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();
    }

    void CreateGraphicPipeline()
    {
        using namespace quark::graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Create shader
        vert_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
            "/Users/xieyhccc/develop/Quark/Examples/Texture/texture.vert.spv");
        frag_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
            "/Users/xieyhccc/develop/Quark/Examples/Texture/texture.frag.spv");

        // Graphic pipeline create info
        GraphicPipeLineDesc pipe_desc;
        pipe_desc.vertShader = vert_shader;
        pipe_desc.fragShader = frag_shader;
        pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
        pipe_desc.topologyType = TopologyType::TRANGLE_LIST;

        // Depth-stencil state
        pipe_desc.depthStencilState = {
            .enableDepthTest = false,
            .enableDepthWrite = false,
            .depthCompareOp = CompareOperation::LESS_OR_EQUAL
        };

        // rasterization state
        pipe_desc.rasterState = {
            .cullMode = CullMode::NONE,
            .polygonMode = PolygonMode::Fill,
            .frontFaceType = FrontFaceType::COUNTER_CLOCKWISE,
        };

        // // Vertex binding info
        VertexBindInfo vert_bind_info = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VertexBindInfo::INPUT_RATE_VERTEX
        };
        pipe_desc.vertexBindInfos.push_back(vert_bind_info);

        // Vertex attributes info
        auto& pos_attrib = pipe_desc.vertexAttribInfos.emplace_back();
        pos_attrib = {
            .binding = 0,
            .format = VertexAttribInfo::ATTRIB_FORMAT_VEC3,
            .location = 0,
            .offset = offsetof(Vertex, position)
        };

        auto& uv_attrib = pipe_desc.vertexAttribInfos.emplace_back();
        uv_attrib = {
            .binding = 0,
            .format = VertexAttribInfo::ATTRIB_FORMAT_VEC3,
            .location = 1,
            .offset = offsetof(Vertex, texCoords)
        };

        graphic_pipeline = graphic_device->CreateGraphicPipeLine(pipe_desc, render_pass_info);
    }


    TextureExample(const std::string& title, const std::string& root, int width, int height)
        :Application(title, root, width, height)
    {
        SetUpRenderPass();
        CreateGraphicPipeline();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateTextureImage();
        CreateSampler();
    }

    ~TextureExample() {};

    void Update(f32 deltaTime) override
    {
        
    }

    void Render(f32 deltaTime) override
    {
        using namespace quark::graphic;

        // auto start = std::chrono::system_clock::now();

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

            // 3. Begin a render pass
            render_pass_info.colorAttachments[0] = swap_chain_image;
            render_pass_info.clearColors[0] = {0.f, 0.f, 0.4f, 1.f};
            cmd->BeginRenderPass(render_pass_info);

            // 4. Bind pipeline and set viewport and scissor
            cmd->BindPipeLine(*graphic_pipeline);

            u32 drawWidth = swap_chain_image->GetDesc().width;
            u32 drawHeight = swap_chain_image->GetDesc().height;
            cmd->SetViewPort(Viewport{.x = 0, .y = 0, .width = (float)drawWidth,
                .height = (float)drawHeight, .minDepth = 0, .maxDepth = 1});
            cmd->SetScissor(Scissor{.extent = {.width = drawWidth, .height = drawHeight},
                .offset = {.x = 0, .y = 0}});

            // 5.Create uniform buffer
            BufferDesc uniform_buffer_desc = {
                .domain = BufferMemoryDomain::CPU,
                .size = sizeof(uniformBufferData),
                .usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT
            };
            Ref<Buffer> uniform_buffer = graphic_device->CreateBuffer(uniform_buffer_desc);

            // Fill uniform buffer
            uniformBufferData& mapped_data = *(uniformBufferData*)uniform_buffer->GetMappedDataPtr();
            mapped_data.modelMatrix = glm::mat4(1.f);
            mapped_data.viewMatrix = glm::mat4(1.f);
            float aspect = (float)Window::Instance()->GetWidth() / Window::Instance()->GetHeight();
            mapped_data.projectionMatrix = glm::perspective(glm::radians(45.f), aspect, 0.1f, 100.f);
            mapped_data.projectionMatrix[1] *= -1; // screen space's Y-axis is opposite of world space's Y-axis

            // 6. Bind uniform buffer, texture, and sampler
            cmd->BindUniformBuffer(0, 0, *uniform_buffer, 0, uniform_buffer->GetDesc().size);
            cmd->BindImage(0, 1, *texture_image, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            cmd->BindSampler(0, 1, *linear_sampler);
            
            // 7.Bind vertex buffer and index buffer
            cmd->BindVertexBuffer(0, *vert_buffer, 0);
            cmd->BindIndexBuffer(*index_buffer, 0, IndexBufferFormat::UINT32);

            // 8. Draw call
            cmd->DrawIndexed(index_buffer->GetDesc().size / sizeof(uint32_t), 1, 0, 0, 0);
            cmd->EndRenderPass();

            // 9. Transit swapchain image to present layout for presenting
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
    return new TextureExample("Trangle"," ", 1200, 800);
    
}