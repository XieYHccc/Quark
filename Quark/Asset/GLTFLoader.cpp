#include "pch.h"
#include "Asset/GLTFLoader.h"
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include "Core/Util/AlignedAlloc.h"
#include "Scene/Scene.h"
#include "Graphic/Device.h"
#include "Scene/Components/TransformCmpt.h"
#include "Scene/Components/MeshCmpt.h"
#include "Scene/Components/CameraCmpt.h"
#include "Asset/ImageLoader.h"
namespace asset {
using namespace graphic;
using namespace scene;

std::unordered_map<std::string, bool> GLTFLoader::supportedExtensions_ = {
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

GLTFLoader::GLTFLoader(graphic::Device* device)
    :device_(device)
{
    CORE_DEBUG_ASSERT(device_ != nullptr)

    // Create default resouces
    defalutLinearSampler_ = device_->CreateSampler(SamplerDesc{.minFilter = SamplerFilter::LINEAR,
        .magFliter = SamplerFilter::LINEAR,
        .addressModeU = SamplerAddressMode::REPEAT,
        .addressModeV = SamplerAddressMode::REPEAT,
        .addressModeW = SamplerAddressMode::REPEAT});

    // defalut error check board image
    constexpr uint32_t black = __builtin_bswap32(0x000000FF);
    constexpr uint32_t magenta = __builtin_bswap32(0xFF00FFFF);
    std::array<uint32_t, 32 * 32 > pixels;
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            pixels[y * 32 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }

    ImageDesc texture_desc = {
        .type = ImageType::TYPE_2D,
        .width = 32,
        .height = 32,
        .depth = 1,
        .format = DataFormat::R8G8B8A8_UNORM,
        .arraySize = 1,
        .mipLevels = 1,
        .initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT
    };
    
    ImageInitData init_data = {
        .rowPitch = 32 * 4,
        .slicePitch = 32 * 32 * 4,
        .data = pixels.data()
    };

    defaultCheckBoardImage_ = device_->CreateImage(texture_desc, &init_data);

    // default White image
    constexpr uint32_t white = __builtin_bswap32(0xFFFFFFFF);
    texture_desc.width = 1;
    texture_desc.height = 1;
    
    init_data = {
        .rowPitch = 1 * 4,
        .slicePitch = 1 * 1 * 4,
        .data = &white
    };

    defaultWhiteImage_ = device_->CreateImage(texture_desc, &init_data);

    // Create defalult texture
    defaultColorTexture_ = CreateRef<scene::Texture>();
    defaultColorTexture_->image = defaultWhiteImage_;
    defaultColorTexture_->sampler = defalutLinearSampler_;
    defaultColorTexture_->SetName("Default color texture");

    defaultMetalTexture_ =CreateRef<scene::Texture>();
    defaultMetalTexture_->image = defaultWhiteImage_;
    defaultMetalTexture_->sampler = defalutLinearSampler_;
    defaultMetalTexture_->SetName("Default metalic roughness texture");
}

Scope<scene::Scene> GLTFLoader::LoadSceneFromFile(const std::string &filename)
{
    CORE_LOGI("Loading GLTF file: {}", filename)
    
	std::string err;
	std::string warn;
	tinygltf::TinyGLTF gltf_loader;

    bool binary = false; //TODO: Support glb loading
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos) {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
    }

    size_t pos = filename.find_last_of('/');
    if (pos == std::string::npos) {
        pos = filename.find_last_of('\\');
    }
    filePath_ = filename.substr(0, pos);

    // TODO:
    gltf_loader.SetImageLoader(loadImageDataFunc, nullptr);

    bool importResult = binary? gltf_loader.LoadBinaryFromFile(&model_, &err, &warn, filename.c_str()) : gltf_loader.LoadASCIIFromFile(&model_, &err, &warn, filename.c_str());

	if (!err.empty()){
		CORE_LOGE("Error loading gltf model: {}.", err);
	}
	if (!warn.empty()){
		CORE_LOGI("{}", warn);
	}
    if (!importResult) {
		CORE_LOGE("Failed to load gltf file {} for {}", filename);
        return nullptr;
	}
	// Check extensions
	for (auto &used_extension : model_.extensionsUsed) {
		auto it = supportedExtensions_.find(used_extension);

		// Check if extension isn't supported by the GLTFLoader
		if (it == supportedExtensions_.end()) {
			// If extension is required then we shouldn't allow the scene to be loaded
			if (std::find(model_.extensionsRequired.begin(), model_.extensionsRequired.end(), used_extension) != model_.extensionsRequired.end())
				CORE_ASSERT_MSG(0, "Cannot load glTF file. Contains a required unsupported extension: " + used_extension)
			else
				CORE_LOGW("glTF file contains an unsupported extension, unexpected results may occur: {}", used_extension)
		}
		else {
			// Extension is supported, so enable it
			LOGI("glTF file contains extension: {}", used_extension);
			it->second = true;
		}
	}

    Scope<scene::Scene> newScene = CreateScope<scene::Scene>("gltf scene"); // name would be overwritten later..
    scene_ = newScene.get();

    // Load samplers
    samplers_.resize(model_.samplers.size());
    for (size_t sampler_index = 0; sampler_index < model_.samplers.size(); sampler_index++) {
        samplers_[sampler_index] = ParseSampler(model_.samplers[sampler_index]);
    }

    // Load images
    images_.resize(model_.images.size());
    for (size_t image_index = 0; image_index < model_.images.size(); image_index++) {
        Ref<Image> newImage = ParseImage(model_.images[image_index]);
        images_[image_index] = newImage;
    }

    // Load textures
    textures_.resize(model_.textures.size());
    for (size_t texture_index = 0; texture_index < model_.textures.size(); texture_index++) {
        auto newTexture = CreateRef<scene::Texture>();

        // Default values
        newTexture->image = defaultWhiteImage_;
        newTexture->sampler = defalutLinearSampler_;

        if (model_.textures[texture_index].source > -1) {
            newTexture->image = images_[model_.textures[texture_index].source];
        }
        if (model_.textures[texture_index].sampler > -1) {
            newTexture->sampler = samplers_[model_.textures[texture_index].sampler];
        }

        textures_[texture_index] = newTexture;
    }

    // Using dynamic uniform buffer for material uniform data
    size_t min_ubo_alignment = device_->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
    size_t dynamic_alignment = sizeof(Material::UniformBufferBlock);
	if (min_ubo_alignment > 0)
		dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);

	size_t buffer_size = (model_.materials.size() + 1) * dynamic_alignment; // additional 1 is for default material
    auto* ubo_data = (Material::UniformBufferBlock*)util::memalign_alloc(dynamic_alignment, buffer_size);
    CORE_DEBUG_ASSERT(ubo_data)

    // Create uniform buffer for material's uniform data
    BufferDesc uniform_buffer_desc = {
        .domain = BufferMemoryDomain::CPU,
        .size = buffer_size,
        .usageBits = BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };

    Ref<graphic::Buffer> materialUniformBuffer = device_->CreateBuffer(uniform_buffer_desc);
    auto* mapped_data = (Material::UniformBufferBlock*)materialUniformBuffer->GetMappedDataPtr();

    // Load materials
    materials_.reserve(model_.materials.size() + 1); // one more default material
    for (size_t material_index = 0; material_index < model_.materials.size(); material_index++) {
        auto newMaterial = ParseMaterial(model_.materials[material_index]);
        auto* ubo = (Material::UniformBufferBlock*)((u64)ubo_data + (material_index * dynamic_alignment));
        *ubo = newMaterial->uniformBufferData;
        newMaterial->uniformBuffer = materialUniformBuffer;
        newMaterial->uniformBufferOffset = material_index * dynamic_alignment;
        materials_.push_back(newMaterial);
    }

    // Create default material
    {
        defaultMaterial_ = CreateRef<Material>();
        defaultMaterial_->alphaMode = Material::AlphaMode::OPAQUE;
        defaultMaterial_->baseColorTexture = defaultColorTexture_;
        defaultMaterial_->metallicRoughnessTexture = defaultMetalTexture_;
        defaultMaterial_->uniformBuffer = materialUniformBuffer;
        defaultMaterial_->uniformBufferOffset = model_.materials.size() * dynamic_alignment;
        
        auto* ubo = (Material::UniformBufferBlock*)((u64)ubo_data + (model_.materials.size() * dynamic_alignment));
        *ubo = defaultMaterial_->uniformBufferData;

        materials_.push_back(defaultMaterial_);
    }

    // data copy
    std::copy(ubo_data, ubo_data + buffer_size, mapped_data);
    util::memalign_free(ubo_data);

    // Load meshes
    meshes_.reserve(model_.meshes.size());
    for (const auto& gltf_mesh : model_.meshes) {
        meshes_.push_back(ParseMesh(gltf_mesh));
    }

    // TODO: scene handling with no default scene
    // TODO: Support gltf file with multiple scenes
    // CORE_ASSERT(model_.scenes.size() == 1)
    const tinygltf::Scene& gltf_scene = model_.scenes[model_.defaultScene > -1 ? model_.defaultScene : 0];
    scene_->SetName(gltf_scene.name);

    // Load nodes
    nodes_.reserve(model_.nodes.size());
    for (const auto& gltf_node : model_.nodes) {
        auto* newNode = ParseNode(gltf_node);
        nodes_.push_back(newNode);
    }

    // Loop node to again to establish hierachy
    for (size_t i = 0; i < model_.nodes.size(); i++) {
        for (const auto& child : model_.nodes[i].children)
            nodes_[i]->AddChild(nodes_[child]);
    }

    // Add root nodes manually
    scene_->rootNode_->ClearChildren();
    for (const auto& node : gltf_scene.nodes) {
        scene_->rootNode_->AddChild(nodes_[node]);
    }

    return newScene;
}

scene::Node* GLTFLoader::ParseNode(const tinygltf::Node& gltf_node)
{
    scene::Node* newNode = scene_->CreateNode(gltf_node.name);
    scene::Entity* entity = newNode->GetEntity();

	// Parse transform component
    scene::TransformCmpt* transform = newNode->GetEntity()->GetComponent<scene::TransformCmpt>();

	if (gltf_node.translation.size() == 3) {
		glm::vec3 translation = glm::make_vec3(gltf_node.translation.data());
		transform->SetPosition(translation);
	}
	if (gltf_node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(gltf_node.rotation.data());
		transform->SetQuat(q);
	}
	if (gltf_node.scale.size() == 3) {
		glm::vec3 scale = glm::make_vec3(gltf_node.scale.data());
		transform->SetScale(scale);
	}
	if (gltf_node.matrix.size() == 16) {
		transform->SetTRSMatrix(glm::make_mat4x4(gltf_node.matrix.data()));
	};

    // Parse mesh component
    if (gltf_node.mesh > -1) {
        scene::MeshCmpt* mesh_cmpt = entity->AddComponent<scene::MeshCmpt>();
        mesh_cmpt->sharedMesh = meshes_[gltf_node.mesh];
    }

    //TODO: Parse camera component

    return newNode;
}

Ref<graphic::Sampler> GLTFLoader::ParseSampler(const tinygltf::Sampler &gltf_sampler)
{
    SamplerDesc desc = {
        .minFilter = convert_min_filter(gltf_sampler.minFilter),
        .magFliter = convert_mag_filter(gltf_sampler.magFilter),
        .addressModeU = convert_wrap_mode(gltf_sampler.wrapS),
        .addressModeV = convert_wrap_mode(gltf_sampler.wrapT)
    };

    return device_->CreateSampler(desc);
}

Ref<graphic::Image> GLTFLoader::ParseImage(const tinygltf::Image& gltf_image)
{
    if (!gltf_image.image.empty()) { // Image embedded in gltf file or loaded with stb
        ImageDesc desc = {
            .width = static_cast<u32>(gltf_image.width),
            .height  = static_cast<u32>(gltf_image.height),
            .depth = 1u,
            .arraySize = 1,     // Only support 1 layer and 1 mipmap level for embedded image
            .mipLevels = 1,
            .format = DataFormat::R8G8B8A8_UNORM,
            .type = ImageType::TYPE_2D,
            .usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT,
            .initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            .generateMipMaps = true // Generate mipmaps for embedded image
        };
        
        
        ImageInitData init_data = {
            .rowPitch = desc.width * 4,
            .slicePitch = desc.width * desc.height * 4,
            .data = gltf_image.image.data()
        };

        return device_->CreateImage(desc, &init_data);
    }
    else { // Image loaded from external file

        std::string image_uri = filePath_ + "/" +gltf_image.uri;
        bool is_ktx = false;
        if (image_uri.find_last_of(".") != std::string::npos) {
            if (image_uri.substr(image_uri.find_last_of(".") + 1) == "ktx") {
                is_ktx = true;
            }
        }
        
        if (is_ktx) {
            ImageLoader image_loader(device_);
            return image_loader.LoadKtx(gltf_image.uri);
        }
    }
    
    CORE_LOGE("GLTFLoader::ParseImage::Failed to load image: {}", gltf_image.uri)
    return defaultCheckBoardImage_;
}

Ref<scene::Material> GLTFLoader::ParseMaterial(const tinygltf::Material& mat)
{
    auto newMaterial = CreateRef<Material>();
    newMaterial->SetName(mat.name);

    newMaterial->alphaMode = Material::AlphaMode::OPAQUE;
    auto find = mat.additionalValues.find("alphaMode");
    if (find != mat.additionalValues.end()) {
        tinygltf::Parameter param = find->second;
        if (param.string_value == "BLEND")
            newMaterial->alphaMode = Material::AlphaMode::TRANSPARENT;
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
    newMaterial->baseColorTexture = defaultColorTexture_;
    newMaterial->metallicRoughnessTexture = defaultMetalTexture_;

    find = mat.values.find("metallicRoughnessTexture");
    if (find != mat.values.end()) {
        newMaterial->metallicRoughnessTexture = textures_[find->second.TextureIndex()];
    }

    find = mat.values.find("baseColorTexture");
    if (find != mat.values.end()) {
        newMaterial->baseColorTexture = textures_[find->second.TextureIndex()];
    }

    return newMaterial;
}

Ref<scene::Mesh> GLTFLoader::ParseMesh(const tinygltf::Mesh& gltf_mesh)
{

    vertices_.clear();
    indices_.clear();

    size_t vertex_count = 0;
    size_t index_count = 0;

    // Get vertex count and index count up-front
    for (const auto& p : gltf_mesh.primitives) {
        vertex_count += model_.accessors[p.attributes.find("POSITION")->second].count;
        if (p.indices > -1) {
            index_count += model_.accessors[p.indices].count;
        }
    }
    vertices_.reserve(vertex_count);
    indices_.reserve(index_count);

    std::vector<Mesh::SubMeshDescriptor> submeshes;
    submeshes.reserve(gltf_mesh.primitives.size());
    
    // loop primitives
    for (auto& p : gltf_mesh.primitives) {
        u32 start_index = indices_.size();
        u32 start_vertex = vertices_.size();
        u32 index_num = 0;
        glm::vec3 min_pos{};
        glm::vec3 max_pos{};

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
            const tinygltf::Accessor &posAccessor = model_.accessors[p.attributes.find("POSITION")->second];
            const tinygltf::BufferView &posView = model_.bufferViews[posAccessor.bufferView];
            buffer_pos = reinterpret_cast<const float *>(&(model_.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
            min_pos = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
            max_pos = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
            CORE_DEBUG_ASSERT(min_pos != max_pos)
            
            if (p.attributes.find("NORMAL") != p.attributes.end()) {
                const tinygltf::Accessor& normAccessor = model_.accessors[p.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& normView = model_.bufferViews[normAccessor.bufferView];
                buffer_normals= reinterpret_cast<const float *>(&(model_.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
            }

            if (p.attributes.find("TEXCOORD_0") != p.attributes.end()) {
                const tinygltf::Accessor &uvAccessor = model_.accessors[p.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView &uvView = model_.bufferViews[uvAccessor.bufferView];
                buffer_texCoords = reinterpret_cast<const float *>(&(model_.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
            }

            if (p.attributes.find("COLOR_0") != p.attributes.end())
            {
                const tinygltf::Accessor& colorAccessor = model_.accessors[p.attributes.find("COLOR_0")->second];
                const tinygltf::BufferView& colorView = model_.bufferViews[colorAccessor.bufferView];
                buffer_colors = &(model_.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]);
                numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                colorComponentType = colorAccessor.componentType;
                CORE_DEBUG_ASSERT(colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                    || colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)

            }

            // Create vertices
            for (size_t i = 0; i < posAccessor.count; i++) {
                Mesh::Vertex v = {};
                v.position = glm::make_vec3(&buffer_pos[i * 3]);
				v.normal = glm::normalize(buffer_normals ? glm::make_vec3(&buffer_normals[i * 3]) : glm::vec3(0.0f));
				v.uv_x = buffer_texCoords? buffer_texCoords[i * 2] : 0.f;
                v.uv_y = buffer_texCoords? buffer_texCoords[i * 2 + 1] : 0.f;
                if (buffer_colors) {
                    if (colorComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
                        const float NORMALIZATION_FACTOR = 65535.0f;
                        const uint16_t* buf = static_cast<const uint16_t*>(buffer_colors);
                        switch (numColorComponents) {
                        case 3: 
                            v.color = glm::vec4(buf[i * 3] / NORMALIZATION_FACTOR,
                                buf[i * 3 + 1] / NORMALIZATION_FACTOR,
                                buf[i * 3 + 2] / NORMALIZATION_FACTOR,
                                1.f);
                            break;
                        case 4:
                            v.color = glm::vec4(buf[i * 4] / NORMALIZATION_FACTOR,
                                buf[i * 4 + 1] / NORMALIZATION_FACTOR,
                                buf[i * 4 + 2] / NORMALIZATION_FACTOR,
                                buf[i * 4 + 3] / NORMALIZATION_FACTOR);
                            break;
                        default:
                            CORE_ASSERT_MSG(0, "Invalid number of color components")
                    }
                    }
                    else {
                        const float* buf = static_cast<const float*>(buffer_colors);
                        switch (numColorComponents) {
                        case 3: 
                            v.color = glm::vec4(glm::make_vec3(&buf[i * 3]), 1.0f);
                            break;
                        case 4:
                            v.color = glm::make_vec4(&buf[i * 4]);
                            break;
                        default:
                            CORE_ASSERT_MSG(0, "Invalid number of color components")
                    }
                    }
                }
                else
                    v.color = glm::vec4(1.0f);

                vertices_.push_back(v);
            }
        }

        // Indices
        {
            const tinygltf::Accessor &accessor = model_.accessors[p.indices];
            const tinygltf::BufferView &bufferView = model_.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model_.buffers[bufferView.buffer];

            index_num = static_cast<uint32_t>(accessor.count);
            const void* data_ptr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = static_cast<const uint32_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    indices_.push_back(buf[index] + start_vertex);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = static_cast<const uint16_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    indices_.push_back(static_cast<uint32_t>(buf[index]) + start_vertex);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = static_cast<const uint8_t*>(data_ptr);
                for (size_t index = 0; index < accessor.count; index++)
                    indices_.push_back(static_cast<uint32_t>(buf[index]) + start_vertex);
                break;
            }
            default:
                CORE_LOGE("Index component type: {} not supported!", accessor.componentType)
                return nullptr;
			}
        }

        auto& new_submesh = submeshes.emplace_back();
        new_submesh.aabb = {min_pos, max_pos},
        new_submesh.count = index_num,
        new_submesh.startIndex = start_index,
        new_submesh.material = p.material > -1? materials_[p.material] : materials_.back();

    }

    // Create mesh
    auto newMesh = CreateRef<Mesh>(vertices_, indices_, submeshes, false);
    newMesh->SetName(gltf_mesh.name);

    return newMesh;
}

}