#include "Quark/qkpch.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Util/StringUtils.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/AssetExtensions.h"
#include "Quark/Asset/MeshImporter.h"
#include "Quark/Asset/TextureImporter.h"
#include "Quark/Asset/MaterialSerializer.h"
#include "Quark/Asset/ImageAssetImporter.h"
#include "Quark/Project/Project.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

namespace quark {
// static std::filesystem::path s_AssetDirectory = "Assets";
// static std::filesystem::path s_AssetRegistryPath = "Assets/AssetRegistry.qkr";

static AssetMetadata s_NullMetadata;

static AssetType  GetAssetTypeFromString(std::string_view assetType)
{
	if (assetType == "None")                   return AssetType::None;
	if (assetType == "Scene")				   return AssetType::SCENE;
	if (assetType == "Mesh")				   return AssetType::MESH;
	if (assetType == "Material")			   return AssetType::MATERIAL;
	if (assetType == "Texture")				   return AssetType::TEXTURE;
	if (assetType == "Shader")				   return AssetType::SHADER;
	if (assetType == "Script")				   return AssetType::SCRIPT;
	if (assetType == "Audio")				   return AssetType::AUDIO;
	if (assetType == "Image")				   return AssetType::IMAGE;
	if (assetType == "Material1")			   return AssetType::MATERIAL1;

	return AssetType::None;
}

static std::string AssetTypeToString(AssetType type)
{
	switch (type) {
	case AssetType::SCENE: 					   return "Scene";
	case AssetType::MESH: 					   return "Mesh";
	case AssetType::MATERIAL: 				   return "Material";
	case AssetType::TEXTURE: 				   return "Texture";
	case AssetType::SHADER: 				   return "Shader";
	case AssetType::SCRIPT: 				   return "Script";
	case AssetType::AUDIO: 					   return "Audio";
	default: 								   return "None";
	}
}

AssetManager::AssetManager()
{
	CreateDefaultAssets();

	QK_CORE_LOGI_TAG("AssetManager", "AssetManager Created");
}

void AssetManager::Init()
{
	m_loadedAssets.clear();
	m_assetMetadata.clear();

	LoadAssetRegistry();
	QK_CORE_LOGI_TAG("AssetManager", "AssetManager Initialized");
}

Ref<Asset> AssetManager::GetAsset(AssetID id)
{
	Ref<Asset> asset = nullptr;

	// builtin assets?
	auto find = m_memoryOnlyAssets.find(id);
	if (find != m_memoryOnlyAssets.end())
		return find->second;
	
	AssetMetadata metadata = GetAssetMetadata(id);
	std::filesystem::path filePath = Project::GetActive()->GetAssetDirectory() / metadata.filePath;
	if (metadata.IsValid()) 
	{
		if (metadata.isDataLoaded)
		{
			asset = m_loadedAssets[id];
			return asset;
		}
		else 
		{
			// Load asset data
			switch (metadata.type) {
			case AssetType::MESH:
			{
				// TODO: Use MeshSerializer instead of MeshImporter
				MeshImporter meshImporter;
				asset = meshImporter.ImportGLTF(filePath);
				break;
			}
			case AssetType::TEXTURE:
			{
				// TODO: Use TextureSerializer instead of TextureImporter
				TextureImporter textureImporter;
				asset = textureImporter.ImportStb(filePath);
				break;
			}
			case AssetType::MATERIAL:
			{
				MaterialSerializer matSerializer;
				Ref<Material> newMat = CreateRef<Material>();
				if (matSerializer.TryLoadData(filePath, newMat))
					asset = newMat;
				break;
			}
			case AssetType::IMAGE:
			{
				ImageAssetImporter imageImporter;
				asset = imageImporter.Import(filePath);
				break;
			}
			case AssetType::MATERIAL1:
			{
				MaterialSerializer matSerializer;
				Ref<MaterialAsset> newMat = CreateRef<MaterialAsset>();
				if (matSerializer.TryLoadData(filePath, newMat))
					asset = newMat;
				break;
			}
			default:
				QK_CORE_VERIFY(0)
				break;
			}

			if (asset)
			{
				asset->SetAssetID(id);
				m_loadedAssets[id] = asset;
				metadata.isDataLoaded = true;
				SetMetadata(id, metadata);
			}
		}
	}

	return asset;
}

bool AssetManager::IsAssetLoaded(AssetID id)
{
	return m_loadedAssets.contains(id);
}

bool AssetManager::IsAssetIdValid(AssetID id)
{
	return GetAssetMetadata(id).IsValid();
}

void AssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
{
	m_memoryOnlyAssets[asset->GetAssetID()] = asset;
}

void AssetManager::RemoveAsset(AssetID id)
{
	if (m_loadedAssets.contains(id))
		m_loadedAssets.erase(id);

	if (m_assetMetadata.contains(id))
		m_assetMetadata.erase(id);
}

AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& filepath)
{
	return GetAssetTypeFromExtension(filepath.extension().string());
	
}

AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
{
	std::string ext = util::string::ToLowerCopy(extension);
	if (s_AssetExtensionMap.find(ext) == s_AssetExtensionMap.end())
	{
		QK_CORE_LOGW_TAG("AssetManger", "No asset type found for extension{0}", ext);
		return AssetType::None;
	}

	return s_AssetExtensionMap.at(ext);
}

AssetType AssetManager::GetAssetTypeFromID(AssetID id)
{
	if (IsAssetIdValid(id))
		return GetAssetMetadata(id).type;
	else
		return AssetType::None;
}

AssetID AssetManager::GetAssetIDFromFilePath(const std::filesystem::path& filepath)
{
	return GetAssetMetadata(filepath).id;
}

std::unordered_set<AssetID> AssetManager::GetAllAssetsWithType(AssetType type)
{
	std::unordered_set<AssetID> result;

	for (auto& [id, metadata] : m_assetMetadata)
	{
		if (metadata.type == type)
			result.insert(id);
	}

	return result;
}

void AssetManager::SetMetadata(AssetID id, AssetMetadata metaData)
{    
	
	m_assetMetadata[metaData.id] = metaData;
	QK_CORE_VERIFY(metaData.IsValid())
}

AssetID AssetManager::ImportAsset(const std::filesystem::path &filepath)
{

	if (auto metadata = GetAssetMetadata(filepath); metadata.IsValid())
	{
		QK_CORE_LOGW_TAG("AssetManager" ,"Asset already Imported with id{0}", uint64_t(metadata.id));
		return metadata.id;
	}

	AssetType type = GetAssetTypeFromPath(filepath);
	if (type == AssetType::None)
	{
		QK_CORE_LOGW_TAG("AssetManager", "Asset file{0} is not supported.", filepath.string());
		return 0;
	}

	AssetMetadata newMetadata;
	newMetadata.id = AssetID();
	newMetadata.filePath = filepath;
	newMetadata.type = type;
	SetMetadata(newMetadata.id, newMetadata);

	return newMetadata.id;
}

AssetMetadata AssetManager::GetAssetMetadata(AssetID id)
{
	if (m_assetMetadata.contains(id))
		return m_assetMetadata[id];

	return s_NullMetadata;
}

AssetMetadata AssetManager::GetAssetMetadata(const std::filesystem::path& filepath)
{
	for (auto& [id, metadata] : m_assetMetadata)
	{
		if (metadata.filePath == filepath)
			return metadata;
	}

	return s_NullMetadata;
}

void AssetManager::LoadAssetRegistry()
{
	auto registryPath = Project::GetActive()->GetAssetRegistryPath();
	if (!FileSystem::Exists(registryPath))
		QK_CORE_VERIFY(0)

	// Parse registry file into yaml
	std::ifstream stream(registryPath);
	QK_CORE_VERIFY(stream)
	std::stringstream strStream;
	strStream << stream.rdbuf();

	YAML::Node data = YAML::Load(strStream.str());

	auto assetMetadata = data["Assets"];
	if (!assetMetadata)
	{
		QK_CORE_LOGW_TAG("AssetManager", "No asset meta data in registry file");
		return;
	}

	// Parse
	for (auto a : assetMetadata)
	{
		std::string filepath = a["FilePath"].as<std::string>();

		AssetMetadata metadata;
		metadata.id = a["Id"].as<uint64_t>();
		metadata.filePath = filepath;
		metadata.type = GetAssetTypeFromString(a["Type"].as<std::string>());

		if (metadata.type == AssetType::None)
		{
			QK_CORE_LOGW_TAG("AssetManager", "Asset type is None for asset{0}, this should't happen", filepath);
			continue;
		}

		if (metadata.id == 0) 
		{
			QK_CORE_LOGW_TAG("AssetManager", "Asset ID is 0 for asset{0}, this should't happen", filepath);
			continue;
		}

		SetMetadata(metadata.id, metadata);

	}

	QK_CORE_LOGI_TAG("AssetManager", "Loaded {0} asset entries", m_assetMetadata.size());
}
void AssetManager::SaveAssetRegistry()
{
	QK_CORE_LOGI_TAG("AssetManager", "Saving asset registry with{0} assets", m_assetMetadata.size());

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Assets" << YAML::BeginSeq;

	for (auto& [id, entry] : m_assetMetadata)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Id" << YAML::Value << entry.id;
		out << YAML::Key << "FilePath" << YAML::Value << entry.filePath.string();
		out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(entry.type);
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream fout(Project::GetActive()->GetAssetRegistryPath());
	fout << out.c_str();
}

void AssetManager::ReloadAssets()
{

}

void AssetManager::CreateDefaultAssets()
{
	// Create defalult texture
	auto& resourceMngr = RenderSystem::Get().GetRenderResourceManager();
	defaultColorTexture = CreateRef<Texture>();
	defaultColorTexture->image = resourceMngr.image_white;
	defaultColorTexture->sampler = resourceMngr.sampler_linear;
	defaultColorTexture->SetName("Default color texture");
	
	defaultMetalTexture = CreateRef<Texture>();
	defaultMetalTexture->image = resourceMngr.image_white;
	defaultMetalTexture->sampler = resourceMngr.sampler_linear;
	defaultMetalTexture->SetName("Default metalic roughness texture");

	defaultMaterial = CreateRef<Material>();
	defaultMaterial->alphaMode = AlphaMode::MODE_OPAQUE;
	defaultMaterial->baseColorTexture = defaultColorTexture;
	defaultMaterial->metallicRoughnessTexture = defaultMetalTexture;
	defaultMaterial->uniformBufferData.baseColorFactor = glm::vec4(1.0f);
	defaultMaterial->uniformBufferData.metalicFactor = 1.0f;
	defaultMaterial->uniformBufferData.roughNessFactor = 1.0f;
	// TODO: Remove hardcoded shader
	defaultMaterial->shaderProgram = RenderSystem::Get().GetRenderResourceManager().GetShaderLibrary().program_staticMesh;
	defaultMaterial->SetName("Default material");
	
	MeshImporter mesh_loader;
	mesh_cube = mesh_loader.ImportGLTF("BuiltInResources/Gltf/cube.gltf");

	// All default assets' id is 1
	defaultColorTexture->SetAssetID(1);
	defaultMetalTexture->SetAssetID(1);
	defaultMaterial->SetAssetID(1);
	mesh_cube->SetAssetID(15);

	AddMemoryOnlyAsset(mesh_cube);
}

}