#pragma once
#include "Quark/RHI/Common.h"

// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

namespace quark {

class Scene;
class Entity;
class MeshAsset;

struct SkeletonAsset;
struct AnimationAsset;

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

    GLTFImporter(Ref<rhi::Device> device);

    void Import(const std::string& file_path, uint32_t flags = 0);

    Ref<Scene> GetScene() { return m_scene; }

    std::vector<Ref<MeshAsset>>& GetMeshes() { return m_meshes; }
    std::vector<Ref<SkeletonAsset>>& GetSkeletons() { return m_skeletons; }
    std::vector<Ref<AnimationAsset>>& GetAnimations() { return m_animations; }

private:
    Ref<rhi::Sampler> ParseSampler(const tinygltf::Sampler& gltf_sampler);
    Ref<rhi::Image> ParseImage(const tinygltf::Image& gltf_image);
    //Ref<Material> ParseMaterial(const tinygltf::Material& gltf_material);
    Ref<MeshAsset> ParseMesh(const tinygltf::Mesh& gltf_mesh);
    Entity* ParseNode(const tinygltf::Node& gltf_node);

    void LoadSkins();
    void LoadAnimations();
    void MatchAnimationsToSkeletons();

    Ref<rhi::Device> m_rhi_device;
    tinygltf::Model m_gltf_model;
    Ref<Scene> m_scene;
    std::string m_filePath;

    std::vector<Ref<rhi::Sampler>> m_samplers;
    std::vector<Ref<rhi::Image>> m_images;
    // std::vector<Ref<Material>> m_Materials;
    std::vector<Ref<MeshAsset>> m_meshes;
    std::vector<Ref<SkeletonAsset>> m_skeletons;
    std::vector<std::unordered_map<int, int>> m_node_to_bone_maps;
    std::vector<Ref<AnimationAsset>> m_animations;
    std::unordered_map<int, int> m_animation_to_skin_map;

    // Supported extensions mapped to whether they are enabled
    static std::unordered_map<std::string, bool> s_SupportedExtensions;

};

}