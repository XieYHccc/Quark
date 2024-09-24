#include "Quark/qkpch.h"
#include "Quark/Asset/GLTFImporter.h"

#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <Quark/Core/Application.h>
#include "Quark/Core/Util/AlignedAlloc.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Renderer/GpuResourceManager.h"
#include "Quark/Asset/TextureImporter.h"
#include "Quark/Asset/AssetManager.h"

namespace quark {
using namespace quark::graphic;

std::unordered_map<std::string, bool> GLTFImporter::s_SupportedExtensions = {
    {"KHR_lights_punctual", false}};

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

GLTFImporter::GLTFImporter()
    :m_GraphicDevice(Application::Get().GetGraphicDevice())
{

}

GLTFImporter::GLTFImporter(graphic::Device* device)
	:m_GraphicDevice(device)
{

}

Ref<Scene> GLTFImporter::Import(const std::string &filename)
{
    CORE_LOGI("Loading GLTF file: {}", filename)
    
	std::string err;
	std::string warn;
	tinygltf::TinyGLTF gltf_loader;

    bool binary = false; //TODO: Support glb loading
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos)
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");

    size_t pos = filename.find_last_of('/');
    if (pos == std::string::npos) 
        pos = filename.find_last_of('\\');

    m_FilePath = filename.substr(0, pos);

    // TODO:
    gltf_loader.SetImageLoader(loadImageDataFunc, nullptr);

    bool importResult = binary? gltf_loader.LoadBinaryFromFile(&m_Model, &err, &warn, filename.c_str()) : gltf_loader.LoadASCIIFromFile(&m_Model, &err, &warn, filename.c_str());

	if (!err.empty()){
		CORE_LOGE("Error loading gltf model: {}.", err);
	}
	if (!warn.empty()){
		CORE_LOGI("{}", warn);
	}
    if (!importResult) 
        return nullptr;

	// Check extensions
	for (auto &used_extension : m_Model.extensionsUsed) 
    {
		auto it = s_SupportedExtensions.find(used_extension);

		// Check if extension isn't supported by the GLTFImporter
		if (it == s_SupportedExtensions.end()) 
        {
			// If extension is required then we shouldn't allow the scene to be loaded
			if (std::find(m_Model.extensionsRequired.begin(), m_Model.extensionsRequired.end(), used_extension) != m_Model.extensionsRequired.end())
				CORE_ASSERT_MSG(0, "Cannot load glTF file. Contains a required unsupported extension: " + used_extension)
			else
				CORE_LOGW("glTF file contains an unsupported extension, unexpected results may occur: {}", used_extension)
		}
		else 
        {
			// Extension is supported, so enable it
			LOGI("glTF file contains extension: {}", used_extension);
			it->second = true;
		}
	}

    m_Scene = CreateRef<Scene>("gltf scene"); // name would be overwritten later..

    // Load samplers
    m_Samplers.resize(m_Model.samplers.size());
    for (size_t sampler_index = 0; sampler_index < m_Model.samplers.size(); sampler_index++)
        m_Samplers[sampler_index] = ParseSampler(m_Model.samplers[sampler_index]);

    // Load images
    m_Images.resize(m_Model.images.size());
    for (size_t image_index = 0; image_index < m_Model.images.size(); image_index++)
    {
        Ref<Image> newImage = ParseImage(m_Model.images[image_index]);
        m_Images[image_index] = newImage;
    }

    // Load textures
    m_Textures.resize(m_Model.textures.size());
    for (size_t texture_index = 0; texture_index < m_Model.textures.size(); texture_index++)
    {
        auto newTexture = CreateRef<Texture>();

        // Default values
        newTexture->image = GpuResourceManager::Get().image_white;
        newTexture->sampler = GpuResourceManager::Get().sampler_linear;

        if (m_Model.textures[texture_index].source > -1) {
            newTexture->image = m_Images[m_Model.textures[texture_index].source];
        }
        if (m_Model.textures[texture_index].sampler > -1) {
            newTexture->sampler = m_Samplers[m_Model.textures[texture_index].sampler];
        }

        m_Textures[texture_index] = newTexture;
    }

    // (deprecated)Using dynamic uniform buffer for material uniform data
 //   size_t min_ubo_alignment = m_GraphicDevice->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
 //   size_t dynamic_alignment = sizeof(Material::UniformBufferBlock);
	//if (min_ubo_alignment > 0)
	//	dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);

	//size_t buffer_size = (m_Model.materials.size() + 1) * dynamic_alignment; // additional 1 is for default material
 //   auto* ubo_data = (Material::UniformBufferBlock*)util::memalign_alloc(dynamic_alignment, buffer_size);
 //   CORE_DEBUG_ASSERT(ubo_data)

 //   // Create uniform buffer for material's uniform data
 //   BufferDesc uniform_buffer_desc = {
 //       .size = buffer_size,
 //       .domain = BufferMemoryDomain::CPU,
 //       .usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT
 //   };

    //Ref<graphic::Buffer> materialUniformBuffer = m_GraphicDevice->CreateBuffer(uniform_buffer_desc);
    //auto* mapped_data = (Material::UniformBufferBlock*)materialUniformBuffer->GetMappedDataPtr();

    // Load materials
    m_Materials.reserve(m_Model.materials.size() + 1); // one more default material
    for (size_t material_index = 0; material_index < m_Model.materials.size(); material_index++) 
    {
        auto newMaterial = ParseMaterial(m_Model.materials[material_index]);
        // auto* ubo = (Material::UniformBufferBlock*)((u64)ubo_data + (material_index * dynamic_alignment));
        // *ubo = newMaterial->uniformBufferData;
        // newMaterial->uniformBuffer = materialUniformBuffer;
        // newMaterial->uniformBufferOffset = material_index * dynamic_alignment;
        m_Materials.push_back(newMaterial);
    }

    // data copy
    // std::copy(ubo_data, ubo_data + buffer_size, mapped_data);
    // util::memalign_free(ubo_data);

    // Load meshes
    m_Meshes.reserve(m_Model.meshes.size());
    for (const auto& gltf_mesh : m_Model.meshes)
        m_Meshes.push_back(ParseMesh(gltf_mesh));

    // TODO: scene handling with no default scene
    // TODO: Support gltf file with multiple scenes
    // CORE_ASSERT(m_Model.scenes.size() == 1)
    const tinygltf::Scene& gltf_scene = m_Model.scenes[m_Model.defaultScene > -1 ? m_Model.defaultScene : 0];
    m_Scene->SetSceneName(gltf_scene.name);

    // Load nodes
    std::vector<Entity*> entities;
    entities.reserve(m_Model.nodes.size());
    for (const auto& gltf_node : m_Model.nodes) 
    {
        auto* newNode = ParseNode(gltf_node);
        entities.push_back(newNode);
    }

    // Loop node again to establish hierachy
    for (size_t i = 0; i < m_Model.nodes.size(); i++) 
    {
        for (const auto& child : m_Model.nodes[i].children)
            entities[i]->GetComponent<RelationshipCmpt>()->AddChildEntity(entities[child]);
    }

    return m_Scene;
}

Entity* GLTFImporter::ParseNode(const tinygltf::Node& gltf_node)
{
    auto* newObj = m_Scene->CreateEntity(gltf_node.name, nullptr);

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

    // Parse mesh component
    if (gltf_node.mesh > -1) 
    {
        MeshCmpt* mesh_cmpt = newObj->AddComponent<MeshCmpt>();
        mesh_cmpt->sharedMesh = m_Meshes[gltf_node.mesh];

        // Material
        MeshRendererCmpt* meshRenderer = newObj->AddComponent<MeshRendererCmpt>();
        meshRenderer->SetMesh(mesh_cmpt->sharedMesh);
        for (size_t i = 0; auto& p : m_Model.meshes[gltf_node.mesh].primitives)
        {
            if (p.material > -1)
				meshRenderer->SetMaterial(i, m_Materials[p.material]);
			else
				meshRenderer->SetMaterial(i, AssetManager::Get().GetDefaultMaterial());

            i++;
        }
    }
    //TODO: Parse camera component

    return newObj;
}

Ref<graphic::Sampler> GLTFImporter::ParseSampler(const tinygltf::Sampler &gltf_sampler)
{
    SamplerDesc desc = 
    {
        .minFilter = convert_min_filter(gltf_sampler.minFilter),
        .magFliter = convert_mag_filter(gltf_sampler.magFilter),
        .addressModeU = convert_wrap_mode(gltf_sampler.wrapS),
        .addressModeV = convert_wrap_mode(gltf_sampler.wrapT)
    };

    return m_GraphicDevice->CreateSampler(desc);
}

Ref<graphic::Image> GLTFImporter::ParseImage(const tinygltf::Image& gltf_image)
{
    if (!gltf_image.image.empty()) { // Image embedded in gltf file or loaded with stb
        ImageDesc desc;
        desc.width = static_cast<u32>(gltf_image.width);
        desc.height = static_cast<u32>(gltf_image.height);
        desc.depth = 1u;
        desc.arraySize = 1;     // Only support 1 layer and 1 mipmap level for embedded image
        desc.mipLevels = 1;
        desc.format = DataFormat::R8G8B8A8_UNORM;
        desc.type = ImageType::TYPE_2D;
        desc.usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT;
        desc.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        desc.generateMipMaps = true;        // Generate mipmaps for embedded image
        
        
        ImageInitData init_data;
        init_data.data = gltf_image.image.data();
        init_data.rowPitch = desc.width * 4;
        init_data.slicePitch = init_data.rowPitch * desc.height;

        return m_GraphicDevice->CreateImage(desc, &init_data);
    }
    else { // Image loaded from external file

        std::string image_uri = m_FilePath + "/" +gltf_image.uri;
        bool is_ktx = false;
        if (image_uri.find_last_of(".") != std::string::npos) {
            if (image_uri.substr(image_uri.find_last_of(".") + 1) == "ktx") {
                is_ktx = true;
            }
        }
        
        if (is_ktx) {
            TextureImporter textureImporter;
            return textureImporter.ImportKtx(gltf_image.uri)->image;
        }
    }
    
    CORE_LOGE("GLTFImporter::ParseImage::Failed to load image: {}", gltf_image.uri)

    return GpuResourceManager::Get().image_checkboard;
}

Ref<Material> GLTFImporter::ParseMaterial(const tinygltf::Material& mat)
{
    auto newMaterial = CreateRef<Material>();
    newMaterial->SetName(mat.name);
    newMaterial->alphaMode = AlphaMode::OPAQUE;
    newMaterial->shaderProgram = GpuResourceManager::Get().GetShaderLibrary().defaultStaticMeshProgram;

    auto find = mat.additionalValues.find("alphaMode");
    if (find != mat.additionalValues.end()) 
    {
        tinygltf::Parameter param = find->second;
        if (param.string_value == "BLEND")
            newMaterial->alphaMode = AlphaMode::TRANSPARENT;
    }

    // fill uniform buffer data
    find = mat.values.find("roughnessFactor");
    if (find != mat.values.end()) {
        newMaterial->uniformBufferData.metalicFactor = static_cast<float>(find->second.Factor());
    }

    find = mat.values.find("metallicFactor");
    if (find != mat.values.end()) {
        newMaterial->uniformBufferData.roughNessFactor = static_cast<float>(find->second.Factor());
    }

    find = mat.values.find("baseColorFactor");
    if (find != mat.values.end()) {
        newMaterial->uniformBufferData.baseColorFactor = glm::make_vec4(find->second.ColorFactor().data());
    }
    
    // Default textures
    newMaterial->baseColorTexture = AssetManager::Get().GetDefaultColorTexture();
    newMaterial->metallicRoughnessTexture = AssetManager::Get().GetDefaultMetalTexture();

    find = mat.values.find("metallicRoughnessTexture");
    if (find != mat.values.end()) {
        newMaterial->metallicRoughnessTexture = m_Textures[find->second.TextureIndex()];
    }

    find = mat.values.find("baseColorTexture");
    if (find != mat.values.end()) {
        newMaterial->baseColorTexture = m_Textures[find->second.TextureIndex()];
    }

    return newMaterial;
}

Ref<Mesh> GLTFImporter::ParseMesh(const tinygltf::Mesh& gltf_mesh)
{
    Ref<Mesh> newMesh = CreateRef<Mesh>();
    newMesh->SetName(gltf_mesh.name);

    size_t vertexCount = 0;
    size_t indexCount = 0;

    // Get vertex count and index count upfront
    for (const auto& p : gltf_mesh.primitives) 
    {
        vertexCount += m_Model.accessors[p.attributes.find("POSITION")->second].count;

        if (p.indices > -1)
            indexCount += m_Model.accessors[p.indices].count;
    }

    newMesh->vertex_positions.reserve(vertexCount);
    newMesh->vertex_uvs.reserve(vertexCount);
    newMesh->vertex_normals.reserve(vertexCount);
    newMesh->vertex_colors.reserve(vertexCount);
    newMesh->indices.reserve(indexCount);

    std::vector<Mesh::SubMeshDescriptor> submeshes;
    submeshes.reserve(gltf_mesh.primitives.size());
    
    // loop primitives
    for (auto& p : gltf_mesh.primitives) 
    {
        u32 start_index = newMesh->indices.size();
        u32 start_vertex = newMesh->vertex_positions.size();
        u32 index_num = 0;
        glm::vec3 min_pos = {};
        glm::vec3 max_pos = {};

        // Vertices
        {
            const float* buffer_pos = nullptr;
			const float* buffer_normals = nullptr;
			const float* buffer_texCoords = nullptr;
			const void* buffer_colors = nullptr;
            
            int colorComponentType;
            u32 numColorComponents;

            // Position attribute is required
            CORE_ASSERT_MSG(p.attributes.find("POSITION") != p.attributes.end(), "Position attribute is required")
            const tinygltf::Accessor &posAccessor = m_Model.accessors[p.attributes.find("POSITION")->second];
            const tinygltf::BufferView &posView = m_Model.bufferViews[posAccessor.bufferView];
            buffer_pos = reinterpret_cast<const float *>(&(m_Model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
            min_pos = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
            max_pos = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
            CORE_DEBUG_ASSERT(min_pos != max_pos)
            
            if (p.attributes.find("NORMAL") != p.attributes.end()) 
            {
                const tinygltf::Accessor& normAccessor = m_Model.accessors[p.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& normView = m_Model.bufferViews[normAccessor.bufferView];
                buffer_normals= reinterpret_cast<const float *>(&(m_Model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
            }

            if (p.attributes.find("TEXCOORD_0") != p.attributes.end()) 
            {
                const tinygltf::Accessor &uvAccessor = m_Model.accessors[p.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView &uvView = m_Model.bufferViews[uvAccessor.bufferView];
                buffer_texCoords = reinterpret_cast<const float *>(&(m_Model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
            }

            if (p.attributes.find("COLOR_0") != p.attributes.end())
            {
                const tinygltf::Accessor& colorAccessor = m_Model.accessors[p.attributes.find("COLOR_0")->second];
                const tinygltf::BufferView& colorView = m_Model.bufferViews[colorAccessor.bufferView];
                buffer_colors = &(m_Model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]);
                numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                colorComponentType = colorAccessor.componentType;
                CORE_DEBUG_ASSERT(colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
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
                            CORE_ASSERT_MSG(0, "Invalid number of color components")
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
                            CORE_ASSERT_MSG(0, "Invalid number of color components")
                        }
                    }
                    newMesh->vertex_colors.push_back(color);
                }
            }
        }

        // Indices
        {
            const tinygltf::Accessor &accessor = m_Model.accessors[p.indices];
            const tinygltf::BufferView &bufferView = m_Model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = m_Model.buffers[bufferView.buffer];

            index_num = static_cast<uint32_t>(accessor.count);
            const void* data_ptr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

            switch (accessor.componentType) 
            {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = static_cast<const uint32_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    newMesh->indices.push_back(buf[index] + start_vertex);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = static_cast<const uint16_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    newMesh->indices.push_back(static_cast<uint32_t>(buf[index]) + start_vertex);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = static_cast<const uint8_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    newMesh->indices.push_back(static_cast<uint32_t>(buf[index]) + start_vertex);
                break;
            }
            default:
                CORE_LOGE("Index component type: {} not supported!", accessor.componentType)
                return nullptr;
			}
        }

        auto& newSubmesh = submeshes.emplace_back();
        newSubmesh.aabb = { min_pos, max_pos };
        newSubmesh.count = index_num;
        newSubmesh.startIndex = start_index;
        newSubmesh.startVertex = start_vertex;
        // new_submesh.material = p.material > -1? m_Materials[p.material] : m_Materials.back();

    }

    newMesh->subMeshes = submeshes;
    newMesh->CalculateAabbs();

    return newMesh;
}

}