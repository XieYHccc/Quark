#pragma once
// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>
#include "Quark/Graphic/Common.h"
#include "Quark/Scene/Resources/Material.h"
#include "Quark/Scene/Resources/Mesh.h"

namespace quark {

namespace graphic {
class Device;
}

class Scene;
class GameObject;
class Entity;
class GLTFLoader {
public:
    GLTFLoader(graphic::Device* device);
    Scope<Scene> LoadSceneFromFile(const std::string& file_path);

private:
    Ref<graphic::Sampler> ParseSampler(const tinygltf::Sampler& gltf_sampler);
    Ref<graphic::Image> ParseImage(const tinygltf::Image& gltf_image);
    Ref<Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<Mesh> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    Entity* ParseNode(const tinygltf::Node& gltf_node);

    graphic::Device* device_;
    tinygltf::Model model_;
    Scene* scene_;
    std::string filePath_;

    Ref<graphic::Sampler> defalutLinearSampler_;
    Ref<graphic::Image> defaultCheckBoardImage_;
    Ref<graphic::Image> defaultWhiteImage_;
    Ref<Texture> defaultColorTexture_;
    Ref<Texture> defaultMetalTexture_;
    Ref<Material> defaultMaterial_;
    
    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> supportedExtensions_;

    // Temporary storage for indexing
    std::vector<Mesh::Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Ref<graphic::Sampler>> samplers_;
    std::vector<Ref<graphic::Image>> images_;
    std::vector<Ref<Texture>> textures_;
    std::vector<Ref<Material>> materials_;
    std::vector<Ref<Mesh>> meshes_;
    std::vector<Entity*> entities_;

};

}