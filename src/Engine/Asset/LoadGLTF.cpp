#include "pch.h"
#include "Asset/LoadGLTF.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "glm/glm.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <stb_image.h>

#include "Renderer/Renderer.h"
#include "GameObject/Components/MeshCmpt.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Asset/AssetManager.h"

VkFilter extract_filter(fastgltf::Filter filter)
{
    switch (filter) {
    // nearest samplers
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return VK_FILTER_NEAREST;

    // linear samplers
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::shared_ptr<asset::Texture> load_image(const std::string path, fastgltf::Asset& asset, fastgltf::Image& image)
{
    int width, height, nrChannels;
    std::shared_ptr<asset::Texture> newTex;

    std::visit(
        fastgltf::visitor {
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                                                    // local files.

                const std::string path(filePath.uri.path().begin(),
                    filePath.uri.path().end()); // Thanks C++.
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    asset::TextureCreateInfo info {data, (u32)width,
                        (u32)height, (u32)nrChannels, false, path, image.name.c_str()};
                    newTex = asset::Texture::AddToPool(info);
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector) {
                unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
                    &width, &height, &nrChannels, 4);
                if (data) {
                    asset::TextureCreateInfo info {data, (u32)width,
                        (u32)height, (u32)nrChannels, false, path, image.name.c_str()};
                    newTex = asset::Texture::AddToPool(info);
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
                                               // specify LoadExternalBuffers, meaning all buffers
                                               // are already loaded into a vector.
                               [](auto& arg) {},
                               [&](fastgltf::sources::Vector& vector) {
                                   unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                       static_cast<int>(bufferView.byteLength),
                                       &width, &height, &nrChannels, 4);
                                   if (data) {
                                        asset::TextureCreateInfo info {data, (u32)width,
                                            (u32)height, (u32)nrChannels, false, path, image.name.c_str()};
                                        newTex = asset::Texture::AddToPool(info);
                                        stbi_image_free(data);
                                   }
                               } },
                    buffer.data);
            },
        },
        image.data);


    return newTex;
}


std::unique_ptr<Scene> loadGltf(const std::filesystem::path& filePath)
{
    std::cout << "Loading GLTF: " << filePath << std::endl;

    Renderer& renderer = Renderer::Instance();

    std::unique_ptr<Scene> scene = std::make_unique<Scene>("gltf");
    Scene& file = *scene.get();

    fastgltf::Parser parser {};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGLTF(&data, filePath.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }
    // ===================================Load Resources================================
    
    // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<asset::Mesh>> meshes;
    std::vector<GameObject*> gameObjects;
    std::vector<std::shared_ptr<asset::Sampler>> samplers;
    std::vector<std::shared_ptr<asset::Texture>> images;
    std::vector<std::shared_ptr<asset::Material>> materials;

    meshes.reserve(gltf.meshes.size());
    images.reserve(gltf.images.size());
    materials.reserve(gltf.materials.size());

    // load samplers
    for (fastgltf::Sampler& sampler : gltf.samplers) {

        VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode= extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        asset::SamplerCreateInfo samplinfo {.info = sampl, .path = filePath.string(), .name = sampler.name.c_str()};
        auto newSampler = asset::Sampler::AddToPool(samplinfo);
        samplers.push_back(newSampler);
    }

    // load all textures
	for (fastgltf::Image& image : gltf.images) {
		auto img = load_image(filePath.string(), gltf, image);

		if (img) {
			images.push_back(img);
		}
		else 
        {
			// we failed to load, so lets give the slot a default white texture to not
			// completely break loading
			images.push_back(asset::Texture::GetFromPool("BuiltIn", "WhiteTexture"));
			CORE_LOG_ERROR("gltf failed to load texture {}", image.name);
		}
	}
    
    // load materials
    for (int data_index = 0; fastgltf::Material& mat : gltf.materials) {
        asset::MaterialCreateInfo matinfo{ .name = mat.name.c_str(), .path = filePath.string()};

        // fill ubo data
        matinfo.bufferData.colorFactors = glm::vec4(mat.pbrData.baseColorFactor[0],
            mat.pbrData.baseColorFactor[1], mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]);
        matinfo.bufferData.metalRoughFactors.x = mat.pbrData.metallicFactor;
        matinfo.bufferData.metalRoughFactors.y = mat.pbrData.roughnessFactor;

        matinfo.mode = AlphaMode::OPAQUE;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            matinfo.mode = AlphaMode::TRANSPARENT;
        }

        // default the material textures
        matinfo.baseColorTex.texture = asset::Texture::GetFromPool("BuiltIn", "ErrorTexture");
        matinfo.baseColorTex.sampler = asset::Sampler::GetFromPool("BuiltIn", "DefaultLinearSampler");
        matinfo.metalRougthTex.texture = asset::Texture::GetFromPool("BuiltIn", "WhiteTexture");
        matinfo.metalRougthTex.sampler = asset::Sampler::GetFromPool("BuiltIn", "DefaultLinearSampler");

        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            matinfo.baseColorTex.texture = images[img];
            matinfo.baseColorTex.sampler = samplers[sampler];
        }

        // build material
        auto newMat = asset::Material::AddToPool(matinfo);
        materials.push_back(newMat);
        // file.materialsMap[mat.name.c_str()] = std::move(newMat);    
        data_index++;
    }

    // use the same vectors for all meshes so that the memory doesnt reallocate as often
    asset::MeshCreateInfo meshInfo;
    meshInfo.path = filePath;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        // clear the mesh arrays each mesh, we dont want to merge them by error
        meshInfo.vertices.clear();
        meshInfo.indices.clear();
        meshInfo.submeshs.clear();
        meshInfo.name = mesh.name;
        for (auto&& p : mesh.primitives) {
            SubMeshDescriptor newSurface;
            newSurface.startIndex = (uint32_t)meshInfo.indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = meshInfo.vertices.size();

            // load indexes
            fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
            meshInfo.indices.reserve(meshInfo.indices.size() + indexaccessor.count);

            fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                [&](std::uint32_t idx) {
                    meshInfo.indices.push_back(idx + initial_vtx);
                });

            // load vertex positions
            fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
            meshInfo.vertices.resize(meshInfo.vertices.size() + posAccessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                [&](glm::vec3 v, size_t index) {
                    Vertex newvtx;
                    newvtx.position = v;
                    newvtx.normal = { 1, 0, 0 };
                    newvtx.color = glm::vec4 { 1.f };
                    newvtx.uv_x = 0;
                    newvtx.uv_y = 0;
                    meshInfo.vertices[initial_vtx + index] = newvtx;
                });

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        meshInfo.vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        meshInfo.vertices[initial_vtx + index].uv_x = v.x;
                        meshInfo.vertices[initial_vtx + index].uv_y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) {
                        meshInfo.vertices[initial_vtx + index].color = v;
                    });
            }

            if (p.materialIndex.has_value()) {
                newSurface.material = materials[p.materialIndex.value()];
            } else {
                newSurface.material = materials[0];
            }

            //loop the vertices of this surface, find min/max bounds
            glm::vec3 minpos = meshInfo.vertices[initial_vtx].position;
            glm::vec3 maxpos = meshInfo.vertices[initial_vtx].position;
            for (int i = initial_vtx; i < meshInfo.vertices.size(); i++) {
                minpos = glm::min(minpos, meshInfo.vertices[i].position);
                maxpos = glm::max(maxpos, meshInfo.vertices[i].position);
            }
            // calculate origin and extents from the min/max, use extent lenght for radius
            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

            meshInfo.submeshs.push_back(newSurface);
        }

        auto newMesh = asset::Mesh::AddToPool( meshInfo);
        // auto newMesh = AssetManager::Instance().AddToPool<asset::Mesh>(filePath.string(), mesh.name.c_str(), meshInfo);
        meshes.push_back(newMesh);
        // file.meshesMap[mesh.name.c_str()] = std::move(newMesh);
    }

    // load all game objects and their children
    for (fastgltf::Node& node : gltf.nodes) {
        GameObject* newObject = file.AddGameObject(node.name.c_str());

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
        if (node.meshIndex.has_value()) {
            auto meshRenderCmpt = newObject->AddComponent<MeshCmpt>();
            meshRenderCmpt->gpuMesh = meshes[*node.meshIndex];
        } 

        std::visit(fastgltf::visitor {
                [&](fastgltf::Node::TransformMatrix matrix) {
                    glm::mat4 trs(1.f);
                    memcpy(&trs, matrix.data(), sizeof(matrix));
                    newObject->transformCmpt->SetTRSMatrix(trs);
                },
                [&](fastgltf::Node::TRS transform) {
                    glm::vec3 tl(transform.translation[0], transform.translation[1],
                        transform.translation[2]);
                    glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                        transform.rotation[2]);
                    glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                    newObject->transformCmpt->SetPosition(tl);
                    newObject->transformCmpt->SetQuat(rot);
                    newObject->transformCmpt->SetScale(sc);
                } 
            },
            node.transform);
            
        gameObjects.push_back(newObject);
    }

    // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        GameObject* object = gameObjects[i];

        for (auto& c : node.children) {
            object->childrens.push_back(gameObjects[c]);
            gameObjects[c]->parent = object;
        }
    }

    return scene;
}