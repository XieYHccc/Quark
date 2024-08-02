#pragma once
// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>
#include "Graphic/Common.h"
#include "Scene/Resources/Material.h"
#include "Scene/Resources/Mesh.h"

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
    Ref<scene::resource::Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<scene::resource::Mesh> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    scene::Node* ParseNode(const tinygltf::Node& gltf_node);

    graphic::Device* device_;
    tinygltf::Model model_;
    scene::Scene* scene_;
    std::string filePath_;

    Ref<graphic::Sampler> defalutLinearSampler_;
    Ref<graphic::Image> defaultCheckBoardImage_;
    Ref<graphic::Image> defaultWhiteImage_;
    Ref<scene::resource::Texture> defaultColorTexture_;
    Ref<scene::resource::Texture> defaultMetalTexture_;
    Ref<scene::resource::Material> defaultMaterial_;
    
    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> supportedExtensions_;

    // Temporary storage for indexing
    std::vector<scene::resource::Mesh::Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Ref<graphic::Sampler>> samplers_;
    std::vector<Ref<graphic::Image>> images_;
    std::vector<Ref<scene::resource::Texture>> textures_;
    std::vector<Ref<scene::resource::Material>> materials_;
    std::vector<Ref<scene::resource::Mesh>> meshes_;
    std::vector<scene::Node*> nodes_;

};

}