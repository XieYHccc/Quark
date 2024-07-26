#include "Asset/MeshLoader.h"
#include "Asset/GLTFLoader.h"
#include "Scene/Components/MeshCmpt.h"
#include "Scene/Scene.h"
namespace asset {
Ref<render::Mesh> MeshLoader::LoadGLTF(const std::string& filepath) {
    GLTFLoader gltf_loader(graphicDevice_);

    Scope<scene::Scene> gltf_scene = gltf_loader.LoadSceneFromFile(filepath);
    if (!gltf_scene) {
        CORE_LOGE("Failed to load gltf scene from file: {}", filepath);
        return nullptr;
    }

    // Assume there is only one mesh in the scene
    CORE_DEBUG_ASSERT(gltf_scene->meshes_.size() == 1)
    return gltf_scene->meshes_[0];

}
}