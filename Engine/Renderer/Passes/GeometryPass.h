#pragma once
#include "Graphics/Vulkan/Assert.h"
#include "GameObject/Components/MeshCmpt.h"

class Scene;
struct PerFrameData;

// This pass is responsible for rendering a Scene.
// In deferred rendering, this pass be combined with other
// passes to render a scene.
class GeometryPass {
public:
    GeometryPass(const char* vert_path, const char* frag_path,
        const std::vector<VkRenderingAttachmentInfo>& colorAttachments,
        const VkRenderingAttachmentInfo& depthAttachment);

    ~GeometryPass();

    void Prepare(Scene* scene);
    void Draw(PerFrameData* data);

    void SetResoluton(const VkExtent2D& resolution) { resolution_ = resolution; }

private:
    void UpdateDrawContext();

    VkPipeline opaquePipeline_;
    VkPipeline transparentPipeline_;
    VkExtent2D resolution_;

    std::vector<VkRenderingAttachmentInfo> colorAttachmentsInfo_;
    VkRenderingAttachmentInfo depthAttachmentInfo_;

    std::vector<MeshCmpt*> meshes_;
    Scene* scene_;
    DrawContext drawContext_;
};