#pragma once
#include "Quark/RHI/Common.h"
#include "Quark/Asset/MeshAsset.h"

// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

namespace quark {

namespace graphic {
class Device;
}

class Scene;
class Entity;

class GLTFImporter {
public:
    enum ImportingFlags 
    {
		None = 0,
		ImportMaterials = 1 << 0,
		ImportTextures = 1 << 1,
		ImportMeshes = 1 << 2,
		ImportNodes = 1 << 3,
		ImportAnimations = 1 << 4,
		ImportAll = ImportMaterials | ImportTextures | ImportMeshes | ImportNodes | ImportAnimations
	};

    GLTFImporter();
    GLTFImporter(Ref<rhi::Device> device);

    void Import(const std::string& file_path, uint32_t flags = 0);

    Ref<Scene> GetScene() { return m_Scene; }

    std::vector<Ref<MeshAsset>>& GetMeshes() { return m_Meshes; }

private:
    Ref<rhi::Sampler> ParseSampler(const tinygltf::Sampler& gltf_sampler);
    Ref<rhi::Image> ParseImage(const tinygltf::Image& gltf_image);
    //Ref<Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<MeshAsset> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    Entity* ParseNode(const tinygltf::Node& gltf_node);

    Ref<rhi::Device> m_GraphicDevice;

    tinygltf::Model m_Model;

    Ref<Scene> m_Scene;
    std::string m_FilePath;

    // Temporary storage for indexing
    std::vector<Ref<rhi::Sampler>> m_Samplers;
    std::vector<Ref<rhi::Image>> m_Images;
    // std::vector<Ref<Texture>> m_Textures;
    // std::vector<Ref<Material>> m_Materials;
    std::vector<Ref<MeshAsset>> m_Meshes;

    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> s_SupportedExtensions;

};

}