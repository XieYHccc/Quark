#pragma once
// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>
#include "Quark/Graphic/Common.h"
#include "Quark/Asset/Material.h"
#include "Quark/Asset/Mesh.h"

namespace quark {

namespace graphic {
class Device;
}

class Scene;
class Entity;
class GLTFImporter {
public:
    GLTFImporter();
    Ref<Scene> Import(const std::string& file_path);

private:
    Ref<graphic::Sampler> ParseSampler(const tinygltf::Sampler& gltf_sampler);
    Ref<graphic::Image> ParseImage(const tinygltf::Image& gltf_image);
    Ref<Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<Mesh> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    Entity* ParseNode(const tinygltf::Node& gltf_node);

    graphic::Device* m_GraphicDevice;

    tinygltf::Model m_Model;

    Ref<Scene> m_Scene;
    std::string m_FilePath;

    // Temporary storage for indexing
    std::vector<Ref<graphic::Sampler>> m_Samplers;
    std::vector<Ref<graphic::Image>> m_Images;
    std::vector<Ref<Texture>> m_Textures;
    std::vector<Ref<Material>> m_Materials;
    std::vector<Ref<Mesh>> m_Meshes;
    std::vector<Entity*> m_Entities;

    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> s_SupportedExtensions;

};

}