#pragma once
// #define TINYGLTF_NO_STB_IMAGE
// #define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>
#include "Graphic/Common.h"
#include "Renderer/RenderTypes.h"

namespace scene {
class Scene;
class Node;
}

namespace graphic {
class Device;
}

namespace asset {

class GLTFLoader {
public:
    GLTFLoader(graphic::Device* device);
    Scope<scene::Scene> LoadSceneFromFile(const std::string& file_path);

private:
    Ref<graphic::Sampler> ParseSampler(const tinygltf::Sampler& gltf_sampler);
    Ref<graphic::Image> ParseImage(const tinygltf::Image& gltf_image);
    Ref<render::Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<render::Mesh> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    scene::Node* ParseNode(const tinygltf::Node& gltf_node);

    graphic::Device* device_;
    tinygltf::Model model_;
    scene::Scene* scene_;
    std::string filePath_;

    Ref<graphic::Sampler> defalutLinearSampler_;
    Ref<graphic::Image> defaultCheckBoardImage_;
    Ref<graphic::Image> defaultWhiteImage_;
    Ref<render::Texture> defaultColorTexture_;
    Ref<render::Texture> defaultMetalTexture_;
    Ref<render::Material> defaultMaterial_;
    
    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> supportedExtensions_;

    // To avoid repeated creation
    std::vector<render::Vertex> vertices_;
    std::vector<u32> indices_;
};

}