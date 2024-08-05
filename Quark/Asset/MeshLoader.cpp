#include "qkpch.h"
#include "Asset/MeshLoader.h"
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#include "Asset/GLTFLoader.h"
#include "Scene/Components/MeshCmpt.h"
#include "Scene/Scene.h"

namespace std {
    template<> struct hash<scene::Mesh::Vertex> {
        size_t operator()(scene::Mesh::Vertex const& vertex) const {
            size_t pos_hash = hash<glm::vec3>()(vertex.position);
            size_t uv_x_hash = hash<float>()(vertex.uv_x);
            size_t normal_hash = hash<glm::vec3>()(vertex.normal);
            size_t uv_y_hash = hash<float>()(vertex.uv_y);
            size_t color_hash = hash<glm::vec4>()(vertex.color);

            return (((((pos_hash ^ (uv_x_hash << 1)) >> 1) ^ (normal_hash << 1)) >> 1) ^ (uv_y_hash << 1)) ^ (color_hash << 1);
        }
    };
}

namespace asset {
Ref<scene::Mesh> MeshLoader::LoadGLTF(const std::string& filepath) {
    GLTFLoader gltf_loader(graphicDevice_);

    Scope<scene::Scene> gltf_scene = gltf_loader.LoadSceneFromFile(filepath);
    if (!gltf_scene) {
        CORE_LOGE("Failed to load gltf scene from file: {}", filepath);
        return nullptr;
    }

    // Assume there is only one mesh in the scene
	auto& meshCmpts = gltf_scene->GetComponents<scene::MeshCmpt>();
	if (meshCmpts.empty()) {
		CORE_LOGE("No mesh component found in gltf scene: {}", filepath);
		return nullptr;
	}

	auto [meshCmpt] = meshCmpts[0];
	return meshCmpt->sharedMesh;

}

Ref<scene::Mesh> MeshLoader::LoadOBJ(const std::string &filepath)
{
    tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig config;
	config.vertex_color = false;
	bool success = reader.ParseFromFile(filepath, config);

	if (!reader.Warning().empty())
		CORE_LOGW(reader.Warning())

	if (!reader.Error().empty())
		CORE_LOGE(reader.Error());

	if (!success)
		return nullptr;

    const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();
	const auto& materials = reader.GetMaterials();

	// // Temporary Caches
	// std::vector<std::shared_ptr<asset::Material>> matArray;

	// // Load materials
	// for (const auto& mat : materials)
	// {
	// 	asset::MaterialCreateInfo matInfo;
	// 	matInfo.baseColorTex.texture = asset::Texture::GetFromPool("BuiltIn", "ErrorTexture");
    //     matInfo.baseColorTex.sampler = asset::Sampler::GetFromPool("BuiltIn", "DefaultLinearSampler");
    //     matInfo.metalRougthTex.texture = asset::Texture::GetFromPool("BuiltIn", "WhiteTexture");
    //     matInfo.metalRougthTex.sampler = asset::Sampler::GetFromPool("BuiltIn", "DefaultLinearSampler");
	//     matInfo.mode = AlphaMode::OPAQUE;
	// 	if(!mat.diffuse_texname.empty())
	// 	{
	// 		matInfo.bufferData.colorFactors = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1);
	// 		matInfo.baseColorTex.texture = std::make_shared<asset::Texture>("../Assets/Textures" + mat.diffuse_texname);
	// 	}
	// 	if(!mat.metallic_texname.empty()) 
	// 	{
	// 		matInfo.bufferData.metalRoughFactors.x = mat.metallic;
	// 		matInfo.bufferData.metalRoughFactors.y = mat.roughness;
	// 		matInfo.metalRougthTex.texture = std::make_shared<asset::Texture>("../Assets/Textures" + mat.metallic_texname);
	// 	}

	// 	auto newMat = asset::Material::AddToPool(matInfo);
	// 	matArray.push_back(newMat);
	// }

    // // helpers
    // std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    // asset::MeshCreateInfo info;

	// // preallocate
	// size_t total_vertices = attrib.vertices.size() / 3;
    // size_t total_indices = 0;
    // for (const auto& shape : shapes) {
    //     total_indices += shape.mesh.indices.size();
    // }

	// info.vertices.reserve(total_vertices);
	// info.indices.reserve(total_indices);
    
    // // Load mesh
	// for (size_t s = 0; s < shapes.size(); ++s) {
    //     SubMeshDescriptor submesh;
    //     submesh.startIndex = info.indices.size();
    //     submesh.count = shapes[s].mesh.num_face_vertices.size() * 3;
	// 	submesh.material = asset::Material::GetFromPool("BuiltIn", "DefaultMaterial");
	// 	if (!matArray.empty() && shapes[s].mesh.material_ids[0] != -1) {
	// 		submesh.material = matArray[shapes[s].mesh.material_ids[0]];
	// 	}

	// 	// Loop over faces(polygon)
	// 	size_t index_offset = 0;
	// 	for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
	// 		auto fv = shapes[s].mesh.num_face_vertices[f];
	// 		if (fv != 3) {
	// 			CORE_LOGE("Quark only support loading triangle meshes in obj files")
	// 			return nullptr;
	// 		}
			
	// 		// Loop over vertices in the face.
	// 		for (size_t v = 0; v < fv; v++) {
    //             Vertex vert;
	// 			vert.color = glm::vec4 { 1.f };
	// 			vert.normal =  { 1, 0, 0 };
	// 			vert.uv_x = 0;
	// 			vert.uv_y = 0;
				
	// 			tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

	// 			// fill position
	// 			tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
	// 			tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
	// 			tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
	// 			vert.position = glm::vec3(vx, vy, vz);
				
	// 			// fill normal
	// 			if (idx.normal_index != -1) {
	// 				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
	// 				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
	// 				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
	// 				vert.normal = glm::vec3(nx, ny, nz);
	// 			}

	// 			// fill uv
	// 			if (idx.texcoord_index != -1) {
	// 				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
	// 				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
	// 				vert.uv_x = tx;
    //                 vert.uv_y = 1.f - ty;
	// 			}

	// 			// add vertex and index
	// 			if (uniqueVertices.count(vert) == 0) {
	// 				uniqueVertices[vert] = static_cast<uint32_t>(info.vertices.size());
	// 				info.vertices.push_back(vert);
	// 			}
    //   			info.indices.push_back(uniqueVertices[vert]);
	// 		}

	// 		index_offset += fv;
    //     }

	// 	//loop the vertices of this surface, find min/max bounds
	// 	glm::vec3 minpos = info.vertices[submesh.startIndex].position;
	// 	glm::vec3 maxpos = info.vertices[submesh.startIndex].position;
	// 	for (int i = submesh.startIndex; i < info.vertices.size(); i++) {
	// 		minpos = glm::min(minpos, info.vertices[i].position);
	// 		maxpos = glm::max(maxpos, info.vertices[i].position);
	// 	}
	// 	// calculate origin and extents from the min/max, use extent lenght for radius
	// 	submesh.bounds.origin = (maxpos + minpos) / 2.f;
	// 	submesh.bounds.extents = (maxpos - minpos) / 2.f;
	// 	submesh.bounds.sphereRadius = glm::length(submesh.bounds.extents);

	// 	info.submeshs.push_back(submesh);
	// }

	// auto mesh = std::make_shared<asset::Mesh>(info);

    // return mesh;

    return nullptr;
}

}