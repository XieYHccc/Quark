#include "Quark/qkpch.h"
#include "Quark/Asset/GLTFImporter.h"
#include "Quark/Asset/ImageAsset.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Animation/SkeletonAsset.h"
#include "Quark/Animation/AnimationAsset.h"
#include "Quark/Core/Application.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Scene/Components/ArmatureComponent.h"
#include "Quark/Render/RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

namespace quark {

    using namespace quark::rhi;

    std::unordered_map<std::string, bool> GLTFImporter::s_SupportedExtensions = {
        {"KHR_lights_punctual", false}
    };

    static bool loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
    {
        // KTX files will be handled by our own code
        if (image->uri.find_last_of(".") != std::string::npos) {
            if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx2") {
                return true;
            }
        }

        return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
    }

    inline SamplerFilter convert_min_filter(int min_filter)
    {
        switch (min_filter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return SamplerFilter::NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return SamplerFilter::LINEAR;
        default:
            return SamplerFilter::LINEAR;
        }
    };

    inline SamplerFilter convert_mag_filter(int mag_filter)
    {
        switch (mag_filter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            return SamplerFilter::NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            return SamplerFilter::LINEAR;
        default:
            return SamplerFilter::LINEAR;
        }
    };

    inline SamplerAddressMode convert_wrap_mode(int wrap)
    {
        switch (wrap)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return SamplerAddressMode::REPEAT;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return SamplerAddressMode::CLAMPED_TO_EDGE;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return SamplerAddressMode::MIRRORED_REPEAT;
        default:
            return SamplerAddressMode::REPEAT;
        }
    };


    GLTFImporter::GLTFImporter(Ref<rhi::Device> device)
        :m_rhi_device(device)
    {

    }

    GLTFImporter::GLTFImporter()
        :m_rhi_device(RenderSystem::Get().GetDevice())
    {

    }

    void GLTFImporter::Import(const std::string& filename, uint32_t flags)
    {
        if (flags == 0)
            return;

        QK_CORE_LOGI_TAG("AssetManager", "Loading GLTF file: {}", filename);

        std::string err;
        std::string warn;
        tinygltf::TinyGLTF gltf_loader;

        bool binary = false;
        size_t extpos = filename.rfind('.', filename.length());
        if (extpos != std::string::npos)
            binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");

        size_t pos = filename.find_last_of('/');
        if (pos == std::string::npos)
            pos = filename.find_last_of('\\');

        m_filePath = filename.substr(0, pos);

        // TODO:
        gltf_loader.SetImageLoader(loadImageDataFunc, nullptr);

        bool importResult = binary ? gltf_loader.LoadBinaryFromFile(&m_gltf_model, &err, &warn, filename.c_str()) : gltf_loader.LoadASCIIFromFile(&m_gltf_model, &err, &warn, filename.c_str());

        if (!err.empty())
            QK_CORE_LOGE_TAG("GLTFImporter", "{}", err);
        if (!warn.empty())
            QK_CORE_LOGI_TAG("GLTFImporter", "{}", warn);
        if (!importResult)
            return;

        // Check extensions
        for (auto& used_extension : m_gltf_model.extensionsUsed)
        {
            auto it = s_SupportedExtensions.find(used_extension);

            // Check if extension isn't supported by the GLTFImporter
            if (it == s_SupportedExtensions.end())
            {
                // If extension is required then we shouldn't allow the scene to be loaded
                if (std::find(m_gltf_model.extensionsRequired.begin(), m_gltf_model.extensionsRequired.end(), used_extension) != m_gltf_model.extensionsRequired.end())
                    QK_CORE_VERIFY(0, "Cannot load glTF file. Contains a required unsupported extension: {}", used_extension)
                else
                    QK_CORE_LOGW_TAG("AssetManger", "glTF file contains an unsupported extension, unexpected results may occur: {}", used_extension);
            }
            else
            {
                // Extension is supported, so enable it
                QK_CORE_LOGI_TAG("AssetManger", "glTF file contains extension: {}", used_extension);
                it->second = true;
            }
        }

        if (flags & ImportingFlags::ImportTextures)
        {
            // Load samplers
            //m_samplers.resize(m_gltf_model.samplers.size());
            //for (size_t sampler_index = 0; sampler_index < m_gltf_model.samplers.size(); sampler_index++)
            //    m_samplers[sampler_index] = ParseSampler(m_gltf_model.samplers[sampler_index]);

            // Load images
            m_images.resize(m_gltf_model.images.size());
            for (size_t image_index = 0; image_index < m_gltf_model.images.size(); image_index++)
            {
                Ref<ImageAsset> newImage = ParseImage(m_gltf_model.images[image_index]);
                m_images[image_index] = newImage;
            }
        }

        if (flags & ImportingFlags::ImportAnimations)
		{
			LoadSkins();
			LoadAnimations();
		}

        // Load materials
        if (flags & ImportingFlags::ImportMaterials)
        {
            m_materials.reserve(m_gltf_model.materials.size());
            for (size_t material_index = 0; material_index < m_gltf_model.materials.size(); material_index++)
                m_materials.push_back(ParseMaterial(m_gltf_model.materials[material_index]));
        }
        // Load meshes
        m_meshes.reserve(m_gltf_model.meshes.size());
        for (const auto& gltf_mesh : m_gltf_model.meshes)
            m_meshes.push_back(ParseMesh(gltf_mesh));

        AddAllAssetsAsMemoryOnly(); // TODO:remove

        // create scene and load nodes
        if (flags & ImportingFlags::ImportNodes)
        {
            m_scene = CreateRef<Scene>("gltf scene"); // name would be overwritten later.

            // TODO: scene handling with no default scene
            // TODO: Support gltf file with multiple scenes
            // CORE_ASSERT(m_gltf_model.scenes.size() == 1)
            const tinygltf::Scene& gltf_scene = m_gltf_model.scenes[m_gltf_model.defaultScene > -1 ? m_gltf_model.defaultScene : 0];
            m_scene->sceneName = gltf_scene.name;

            // Load nodes
            std::vector<Entity*> entities;
            entities.reserve(m_gltf_model.nodes.size());
            for (const auto& gltf_node : m_gltf_model.nodes)
            {
                auto* newNode = ParseNode(gltf_node);
                entities.push_back(newNode);
            }

            // Loop node again to establish hierachy
            for (size_t i = 0; i < m_gltf_model.nodes.size(); i++)
            {
                for (const auto& child : m_gltf_model.nodes[i].children)
                    entities[i]->GetComponent<RelationshipCmpt>()->AddChildEntity(entities[child]);
            }
        }
    }

    void GLTFImporter::AddAllAssetsAsMemoryOnly()
    {
        for (auto& mesh : m_meshes)
			AssetManager::Get().AddMemoryOnlyAsset(mesh);

		for (auto& skeleton : m_skeletons)
			AssetManager::Get().AddMemoryOnlyAsset(skeleton);

		for (auto& animation : m_animations)
			AssetManager::Get().AddMemoryOnlyAsset(animation);

		for (auto& material : m_materials)
			AssetManager::Get().AddMemoryOnlyAsset(material);

		for (auto& image : m_images)
			AssetManager::Get().AddMemoryOnlyAsset(image);
    }

    Entity* GLTFImporter::ParseNode(const tinygltf::Node& gltf_node)
    {
        auto* newObj = m_scene->CreateEntity(gltf_node.name, nullptr);

        // Parse transform component
        TransformCmpt* transform = newObj->GetComponent<TransformCmpt>();

        if (gltf_node.translation.size() == 3)
        {
            glm::vec3 translation = glm::make_vec3(gltf_node.translation.data());
            transform->SetLocalPosition(translation);
        }

        if (gltf_node.rotation.size() == 4)
        {
            glm::quat q = glm::make_quat(gltf_node.rotation.data());
            transform->SetLocalRotate(q);
        }

        if (gltf_node.scale.size() == 3)
        {
            glm::vec3 scale = glm::make_vec3(gltf_node.scale.data());
            transform->SetLocalScale(scale);
        }

        if (gltf_node.matrix.size() == 16)
        {
            transform->SetLocalMatrix(glm::make_mat4x4(gltf_node.matrix.data()));
        };

        // mesh
        if (gltf_node.mesh > -1)
        {
            MeshCmpt* mesh_cmpt = newObj->AddComponent<MeshCmpt>();
            mesh_cmpt->sharedMesh = m_meshes[gltf_node.mesh];
            m_scene->AddStaticMeshComponent(newObj, m_meshes[gltf_node.mesh]);
        }
        
        // skeleton
        if (gltf_node.skin > -1)
        {
            Ref<SkeletonAsset> skeleton = m_skeletons[gltf_node.skin];
            // m_scene->AddArmatureComponent(newObj, skeleton);
        }

        //TODO: Parse camera component

        return newObj;
    }

    void GLTFImporter::LoadSkins()
    {
        m_skeletons.resize(m_gltf_model.skins.size());
        m_node_to_bone_maps.resize(m_gltf_model.skins.size());

        for (size_t i = 0; i < m_gltf_model.skins.size(); i++)
        {
            Ref<SkeletonAsset> new_skeleton = CreateRef<SkeletonAsset>();

            const tinygltf::Skin& gltf_skin = m_gltf_model.skins[i];
            new_skeleton->skeleton_name = gltf_skin.name;

            // Bone names
            for (int joint : gltf_skin.joints)
                new_skeleton->bone_names.push_back(m_gltf_model.nodes[joint].name);

            // Parent indices and bone hierarchy
            std::unordered_map<int, int>& node_to_bone_map = m_node_to_bone_maps[i];
            for (size_t j = 0; j < gltf_skin.joints.size(); ++j)
                node_to_bone_map[gltf_skin.joints[j]] = j;

            new_skeleton->parent_bone_indices.resize(new_skeleton->bone_names.size(), SkeletonAsset::null_index);
            for (size_t j = 0; j < gltf_skin.joints.size(); ++j)
            {
                int node_index = gltf_skin.joints[j];
                const auto& node = m_gltf_model.nodes[node_index];

                if (node.children.empty()) continue;

                for (int child : node.children)
                {
                    if (node_to_bone_map.count(child))
                        new_skeleton->parent_bone_indices[node_to_bone_map[child]] = j;
                }
            }

            new_skeleton->root_bone_index = node_to_bone_map[gltf_skin.skeleton];

            // Inverse bind matrices
            if (gltf_skin.inverseBindMatrices > -1)
            {
                const tinygltf::Accessor& accessor = m_gltf_model.accessors[gltf_skin.inverseBindMatrices];
                const tinygltf::BufferView& bufferView = m_gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = m_gltf_model.buffers[bufferView.buffer];
                new_skeleton->inverse_bind_matrices.resize(accessor.count);
                memcpy(new_skeleton->inverse_bind_matrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
            }

            // Load initial transformations (translations, rotations, scales)
            new_skeleton->bone_translations.resize(new_skeleton->bone_names.size(), glm::vec3(0.0f));
            new_skeleton->bone_rotations.resize(new_skeleton->bone_names.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            new_skeleton->bone_scales.resize(new_skeleton->bone_names.size(), glm::vec3(1.0f));

            for (size_t j = 0; j < gltf_skin.joints.size(); ++j) 
            {
                const auto& node = m_gltf_model.nodes[gltf_skin.joints[j]];
                if (!node.translation.empty())
                    new_skeleton->bone_translations[j] = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
                if (!node.rotation.empty())
                    new_skeleton->bone_rotations[j] = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
                if (!node.scale.empty())
                    new_skeleton->bone_scales[j] = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
            }

            m_skeletons[i] = new_skeleton;
        }
    }

    void GLTFImporter::LoadAnimations()
    {
        MatchAnimationsToSkeletons();

        m_animations.resize(m_gltf_model.animations.size());

        for (size_t i = 0; i < m_gltf_model.animations.size(); i++)
        {
            tinygltf::Animation& gltf_anim = m_gltf_model.animations[i];
            Ref<AnimationAsset> new_anim = CreateRef<AnimationAsset>();
            new_anim->name = gltf_anim.name;
            int skin_index = m_animation_to_skin_map[i];
            std::unordered_map<int, int>& node_to_bone_map = m_node_to_bone_maps[skin_index];

            // samplers
            new_anim->samplers.resize(gltf_anim.samplers.size());
            for (size_t j = 0; j < gltf_anim.samplers.size(); j++)
            {
                tinygltf::AnimationSampler gltf_sampler = gltf_anim.samplers[j];
                AnimationSampler& dstSampler = new_anim->samplers[j];
                dstSampler.interpolation = gltf_sampler.interpolation;

                // read sampler keyframe input time values
                {
                    const tinygltf::Accessor& accessor = m_gltf_model.accessors[gltf_sampler.input];
                    const tinygltf::BufferView& bufferView = m_gltf_model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = m_gltf_model.buffers[bufferView.buffer];
                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                    const float* buf = static_cast<const float*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++)
                    {
                        dstSampler.inputs.push_back(buf[index]);
                    }
                    // adjust animation's start and end times
                    for (auto input : dstSampler.inputs)
                    {
                        if (input < new_anim->start)
                        {
                            new_anim->start = input;
                        };
                        if (input > new_anim->end)
                        {
                            new_anim->end = input;
                        }
                    }
                }

                // read sampler keyframe output translate/rotate/scale values
                {
                    const tinygltf::Accessor& accessor = m_gltf_model.accessors[gltf_sampler.output];
                    const tinygltf::BufferView& bufferView = m_gltf_model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = m_gltf_model.buffers[bufferView.buffer];
                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                    switch (accessor.type)
                    {
                    case TINYGLTF_TYPE_VEC3: {
                        const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            dstSampler.outputsVec4.push_back(buf[index]);
                        }
                        break;
                    }
                    default: {
                        std::cout << "unknown type" << std::endl;
                        break;
                    }
                    }
                }
            }

            // channels
            new_anim->channels.resize(gltf_anim.channels.size());
            for (size_t j = 0; j < gltf_anim.channels.size(); j++)
            {
                tinygltf::AnimationChannel gltf_channel = gltf_anim.channels[j];
                AnimationChannel& dstChannel = new_anim->channels[j];
                dstChannel.path = gltf_channel.target_path;
                dstChannel.boneIndex = node_to_bone_map[gltf_channel.target_node];
                dstChannel.samplerIndex = gltf_channel.sampler;
            }

            m_animations[i] = new_anim;
        }

    }

    void GLTFImporter::MatchAnimationsToSkeletons()
    {
        for (size_t animationIndex = 0; animationIndex < m_gltf_model.animations.size(); ++animationIndex) {
            const auto& animation = m_gltf_model.animations[animationIndex];

            // Collect all target nodes for the animation
            std::unordered_set<int> animatedNodes;
            for (const auto& channel : animation.channels) 
            {
                animatedNodes.insert(channel.target_node);
            }

            // Match these nodes with the joints of each skin
            for (size_t skinIndex = 0; skinIndex < m_gltf_model.skins.size(); ++skinIndex) {
                const auto& skin = m_gltf_model.skins[skinIndex];

                // Check if any animated node is a joint of this skin
                for (int joint : skin.joints) 
                {
                    if (animatedNodes.count(joint)) 
                    {
                        m_animation_to_skin_map[animationIndex] = skinIndex;
                        break;
                    }
                }
            }
        }

    }

    //Ref<rhi::Sampler> GLTFImporter::ParseSampler(const tinygltf::Sampler& gltf_sampler)
    //{
    //    SamplerDesc desc =
    //    {
    //        .minFilter = convert_min_filter(gltf_sampler.minFilter),
    //        .magFliter = convert_mag_filter(gltf_sampler.magFilter),
    //        .addressModeU = convert_wrap_mode(gltf_sampler.wrapS),
    //        .addressModeV = convert_wrap_mode(gltf_sampler.wrapT)
    //    };

    //    return m_rhi_device->CreateSampler(desc);
    //}

    Ref<ImageAsset> GLTFImporter::ParseImage(const tinygltf::Image& gltf_image)
    {
        auto new_image = CreateRef<ImageAsset>();
        if (!gltf_image.image.empty()) { // Image embedded in gltf file or loaded with stb
            new_image->width = static_cast<u32>(gltf_image.width);
            new_image->height = static_cast<u32>(gltf_image.height);
            new_image->depth = 1u;
            new_image->arraySize = 1;     // Only support 1 layer and 1 mipmap level for embedded image
            new_image->mipLevels = 1;
            new_image->format = DataFormat::R8G8B8A8_UNORM;
            new_image->type = ImageType::TYPE_2D;
            new_image->data.resize(new_image->width * new_image->height * 4);
            memcpy(new_image->data.data(), gltf_image.image.data(), new_image->width * new_image->height * 4);

            ImageInitData init_data;
            init_data.data = new_image->data.data();
            init_data.rowPitch = new_image->width * 4;
            init_data.slicePitch = init_data.rowPitch * new_image->height;
            new_image->slices.push_back(init_data);

            return new_image;
        }
        else { // Image loaded from external file
            QK_CORE_ASSERT(0);
            std::string image_uri = m_filePath + "/" + gltf_image.uri;
            bool is_ktx = false;
            if (image_uri.find_last_of(".") != std::string::npos) {
                if (image_uri.substr(image_uri.find_last_of(".") + 1) == "ktx") {
                    is_ktx = true;
                }
            }
        }

        QK_CORE_LOGW_TAG("AssetManger", "GLTFImporter::ParseImage::Failed to load image: {}", gltf_image.uri);
        return nullptr;
    }

    Ref<MaterialAsset> GLTFImporter::ParseMaterial(const tinygltf::Material& mat)
    {
        auto newMaterial = CreateRef<MaterialAsset>();
        newMaterial->SetName(mat.name);
        newMaterial->alphaMode = AlphaMode::MODE_OPAQUE;
    
        auto find = mat.additionalValues.find("alphaMode");
        if (find != mat.additionalValues.end()) 
        {
            tinygltf::Parameter param = find->second;
            if (param.string_value == "BLEND")
                newMaterial->alphaMode = AlphaMode::MODE_TRANSPARENT;
        }
    
        // fill uniform buffer data
        find = mat.values.find("roughnessFactor");
        if (find != mat.values.end()) {
            newMaterial->metalicFactor = static_cast<float>(find->second.Factor());
        }
    
        find = mat.values.find("metallicFactor");
        if (find != mat.values.end()) {
            newMaterial->roughNessFactor = static_cast<float>(find->second.Factor());
        }
    
        find = mat.values.find("baseColorFactor");
        if (find != mat.values.end()) {
            newMaterial->baseColorFactor = glm::make_vec4(find->second.ColorFactor().data());
        }
        
 
        find = mat.values.find("metallicRoughnessTexture");
        if (find != mat.values.end()) 
        {
            int textureIndex = find->second.TextureIndex();
            Ref<ImageAsset> img = m_images[m_gltf_model.textures[textureIndex].source];
            newMaterial->metallicRoughnessImage = img->GetAssetID();
        }
    
        find = mat.values.find("baseColorTexture");
        if (find != mat.values.end()) 
        {
            int textureIndex = find->second.TextureIndex();
            Ref<ImageAsset> img = m_images[m_gltf_model.textures[textureIndex].source];
            newMaterial->baseColorImage = img->GetAssetID();
        }
    
        return newMaterial;
    }

    Ref<MeshAsset> GLTFImporter::ParseMesh(const tinygltf::Mesh& gltf_mesh)
    {
        Ref<MeshAsset> newMesh = CreateRef<MeshAsset>();
        newMesh->SetName(gltf_mesh.name);

        size_t vertexCount = 0;
        size_t indexCount = 0;

        // Get vertex count and index count upfront
        for (const auto& p : gltf_mesh.primitives)
        {
            vertexCount += m_gltf_model.accessors[p.attributes.find("POSITION")->second].count;

            if (p.indices > -1)
                indexCount += m_gltf_model.accessors[p.indices].count;
        }

        newMesh->vertex_positions.reserve(vertexCount);
        newMesh->vertex_uvs.reserve(vertexCount);
        newMesh->vertex_normals.reserve(vertexCount);
        newMesh->vertex_colors.reserve(vertexCount);
        newMesh->indices.reserve(indexCount);

        std::vector<MeshAsset::SubMeshDescriptor> submeshes;
        submeshes.reserve(gltf_mesh.primitives.size());

        // loop primitives
        for (auto& p : gltf_mesh.primitives)
        {
            size_t start_index = newMesh->indices.size();
            size_t start_vertex = newMesh->vertex_positions.size();
            size_t index_num = 0;
            glm::vec3 min_pos = {};
            glm::vec3 max_pos = {};

            // Vertices
            {
                const float* buffer_pos = nullptr;
                const float* buffer_normals = nullptr;
                const float* buffer_texCoords = nullptr;
                const void* buffer_colors = nullptr;
                const uint16_t* buffer_joint_indices = nullptr;
                const float* buffer_joint_weights = nullptr;
                int colorComponentType;
                u32 numColorComponents;

                // Position attribute is required
                QK_CORE_ASSERT(p.attributes.find("POSITION") != p.attributes.end(), "Position attribute is required");
                const tinygltf::Accessor& posAccessor = m_gltf_model.accessors[p.attributes.find("POSITION")->second];
                const tinygltf::BufferView& posView = m_gltf_model.bufferViews[posAccessor.bufferView];
                buffer_pos = reinterpret_cast<const float*>(&(m_gltf_model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                min_pos = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                max_pos = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                QK_CORE_ASSERT(min_pos != max_pos);

                if (p.attributes.find("NORMAL") != p.attributes.end())
                {
                    const tinygltf::Accessor& normAccessor = m_gltf_model.accessors[p.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normView = m_gltf_model.bufferViews[normAccessor.bufferView];
                    buffer_normals = reinterpret_cast<const float*>(&(m_gltf_model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                }

                if (p.attributes.find("TEXCOORD_0") != p.attributes.end())
                {
                    const tinygltf::Accessor& uvAccessor = m_gltf_model.accessors[p.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvView = m_gltf_model.bufferViews[uvAccessor.bufferView];
                    buffer_texCoords = reinterpret_cast<const float*>(&(m_gltf_model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                }

                if (p.attributes.find("JOINTS_0") != p.attributes.end())
                {
                    const tinygltf::Accessor& jointAccessor = m_gltf_model.accessors[p.attributes.find("JOINTS_0")->second];
                    const tinygltf::BufferView& jointView = m_gltf_model.bufferViews[jointAccessor.bufferView];
                    buffer_joint_indices = reinterpret_cast<const uint16_t*>(&(m_gltf_model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
                }

                if (p.attributes.find("WEIGHTS_0") != p.attributes.end())
                {
                    const tinygltf::Accessor& weightAccessor = m_gltf_model.accessors[p.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& weightView = m_gltf_model.bufferViews[weightAccessor.bufferView];
					buffer_joint_weights = reinterpret_cast<const float*>(&(m_gltf_model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
                }

                if (p.attributes.find("COLOR_0") != p.attributes.end())
                {
                    const tinygltf::Accessor& colorAccessor = m_gltf_model.accessors[p.attributes.find("COLOR_0")->second];
                    const tinygltf::BufferView& colorView = m_gltf_model.bufferViews[colorAccessor.bufferView];
                    buffer_colors = &(m_gltf_model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]);
                    numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                    colorComponentType = colorAccessor.componentType;
                    QK_CORE_ASSERT(colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                        || colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)

                }

                // Create vertices
                for (size_t i = 0; i < posAccessor.count; i++)
                {
                    newMesh->vertex_positions.push_back(glm::make_vec3(&buffer_pos[i * 3]));

                    if (buffer_normals)
                        newMesh->vertex_normals.push_back(glm::normalize(glm::make_vec3(&buffer_normals[i * 3])));

                    if (buffer_texCoords)
                        newMesh->vertex_uvs.push_back(glm::make_vec2(&buffer_texCoords[i * 2]));

                    if (buffer_joint_indices)
                        newMesh->vertex_bone_indices.push_back(glm::vec4(glm::make_vec4(&buffer_joint_indices[i * 4])));
                    
                    if (buffer_joint_weights)
                        newMesh->vertex_bone_weights.push_back(glm::make_vec4(&buffer_joint_weights[i * 4]));

                    if (buffer_colors)
                    {
                        glm::vec4 color = glm::vec4(1.0f);

                        if (colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
                        {
                            const float NORMALIZATION_FACTOR = 65535.0f;
                            const uint16_t* buf = static_cast<const uint16_t*>(buffer_colors);

                            switch (numColorComponents) {
                            case 3:
                                color = glm::vec4(buf[i * 3] / NORMALIZATION_FACTOR,
                                    buf[i * 3 + 1] / NORMALIZATION_FACTOR,
                                    buf[i * 3 + 2] / NORMALIZATION_FACTOR,
                                    1.f);
                                break;
                            case 4:
                                color = glm::vec4(buf[i * 4] / NORMALIZATION_FACTOR,
                                    buf[i * 4 + 1] / NORMALIZATION_FACTOR,
                                    buf[i * 4 + 2] / NORMALIZATION_FACTOR,
                                    buf[i * 4 + 3] / NORMALIZATION_FACTOR);
                                break;
                            default:
                                QK_CORE_VERIFY(0, "Invalid number of color components")
                            }
                        }
                        else
                        {
                            const float* buf = static_cast<const float*>(buffer_colors);
                            switch (numColorComponents) {
                            case 3:
                                color = glm::vec4(glm::make_vec3(&buf[i * 3]), 1.0f);
                                break;
                            case 4:
                                color = glm::make_vec4(&buf[i * 4]);
                                break;
                            default:
                                QK_CORE_VERIFY(0, "Invalid number of color components")
                                    break;
                            }
                        }
                        newMesh->vertex_colors.push_back(color);
                    }
                }
            }

            // Indices
            {
                const tinygltf::Accessor& accessor = m_gltf_model.accessors[p.indices];
                const tinygltf::BufferView& bufferView = m_gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = m_gltf_model.buffers[bufferView.buffer];

                index_num = static_cast<uint32_t>(accessor.count);
                const void* data_ptr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType)
                {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t* buf = static_cast<const uint32_t*>(data_ptr);
                    for (size_t index = 0; index < accessor.count; index++)
                        newMesh->indices.push_back(buf[index] + (uint32_t)start_vertex);
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t* buf = static_cast<const uint16_t*>(data_ptr);
                    for (size_t index = 0; index < accessor.count; index++)
                        newMesh->indices.push_back(static_cast<uint32_t>(buf[index]) + (uint32_t)start_vertex);
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t* buf = static_cast<const uint8_t*>(data_ptr);
                    for (size_t index = 0; index < accessor.count; index++)
                        newMesh->indices.push_back(static_cast<uint32_t>(buf[index]) + (uint32_t)start_vertex);
                    break;
                }
                default:
                    QK_CORE_LOGW_TAG("AssetManger", "Index component type: {} not supported!", accessor.componentType);
                    return nullptr;
                }
            }

            auto& newSubmesh = submeshes.emplace_back();
            newSubmesh.aabb = { min_pos, max_pos };
            newSubmesh.count = (uint32_t)index_num;
            newSubmesh.startIndex = (uint32_t)start_index;
            newSubmesh.startVertex = (uint32_t)start_vertex;
            newSubmesh.materialID = p.material > -1 ? m_materials[p.material]->GetAssetID() : AssetID(0);
        }

        newMesh->subMeshes = submeshes;
        newMesh->CalculateAabbs();

        return newMesh;
    }

}