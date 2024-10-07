#include "Quark/qkpch.h"
#include "Quark/Scene/SceneSerializer.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Util/SerializationUtils.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"

namespace quark {
	
static void SerializeEntity(YAML::Emitter& out, Entity* entity)
{
	out << YAML::BeginMap; // Entity
	out << YAML::Key << "Entity" << YAML::Value << entity->GetComponent<IdCmpt>()->id;

	if (entity->HasComponent<RelationshipCmpt>()) 
	{
		auto* relationshipCmpt = entity->GetComponent<RelationshipCmpt>();
		out << YAML::Key << "Parent";

		if (relationshipCmpt->GetParentEntity() != nullptr) 
			out << YAML::Value << relationshipCmpt->GetParentEntity()->GetComponent<IdCmpt>()->id;
		else
			out << YAML::Value << 0;

		out << YAML::Key << "Children";
		out << YAML::BeginSeq;

		for (auto* child : relationshipCmpt->GetChildEntities())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Id" << YAML::Value << child->GetComponent<IdCmpt>()->id;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
	}

	if (entity->HasComponent<NameCmpt>()) 
	{
		out << YAML::Key << "NameComponent";
		out << YAML::Value << entity->GetComponent<NameCmpt>()->name;
	}

	if (entity->HasComponent<TransformCmpt>())
	{
		out << YAML::Key << "TransformComponent";
		out << YAML::BeginMap; // TransformComponent

		auto* transformCmpt = entity->GetComponent<TransformCmpt>();
		out << YAML::Key << "Position" << YAML::Value << transformCmpt->GetLocalPosition();
		out << YAML::Key << "Rotation" << YAML::Value << glm::degrees(glm::eulerAngles(transformCmpt->GetLocalRotate()));
		out << YAML::Key << "Scale" << YAML::Value << transformCmpt->GetLocalScale();

		out << YAML::EndMap; // TransformComponent
	}

	if (entity->HasComponent<CameraCmpt>())
	{
		out << YAML::Key << "CameraComponent";
		out << YAML::BeginMap; // CameraComponent

		auto* cameraCmpt = entity->GetComponent<CameraCmpt>();
		out << YAML::Key << "Fov" << YAML::Value << cameraCmpt->fov;
		out << YAML::Key << "Near" << YAML::Value << cameraCmpt->zNear;
		out << YAML::Key << "Far" << YAML::Value << cameraCmpt->zFar;
		out << YAML::Key << "Aspect" << YAML::Value << cameraCmpt->aspect;

		out << YAML::EndMap; // CameraComponent
	}

	if (entity->HasComponent<MeshCmpt>())
	{
		auto* meshCmpt = entity->GetComponent<MeshCmpt>();
		out << YAML::Key << "MeshComponent";

		out << YAML::BeginMap; 
		out << YAML::Key << "AssetID" << YAML::Value << meshCmpt->sharedMesh->GetAssetID();
		out << YAML::EndMap;
	}
		
	if (entity->HasComponent<MeshRendererCmpt>())
	{
		auto* meshRendererCmpt = entity->GetComponent<MeshRendererCmpt>();
		out << YAML::Key << "MeshRendererComponent";
		out << YAML::BeginMap; // MeshRendererComponent

		out << YAML::Key << "Materials";
		out << YAML::BeginSeq;

		for (const auto mat : meshRendererCmpt->GetMaterials())
		{
			out << YAML::BeginMap;
			QK_SERIALIZE_PROPERTY_ASSET(AssetID, mat, out);
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap; // MeshRendererComponent
	}
	out << YAML::EndMap; // Entity
}

SceneSerializer::SceneSerializer(Ref<Scene>& scene)
	: m_Scene(scene)
{

}

void SceneSerializer::Serialize(const std::filesystem::path& filepath)
{

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Scene" << YAML::Value << m_Scene->sceneName;
	out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

	for (auto* e : m_Scene->GetAllEntitiesWith<IdCmpt, RelationshipCmpt>()) 
		SerializeEntity(out, e);
	
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream fout(filepath);
	fout << out.c_str();


}

void SceneSerializer::SerializeBinary(AssetID scene)
{
	// Not implemented
	QK_CORE_VERIFY(false);
}

bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
{
	YAML::Node data = YAML::LoadFile(filepath.string());

	try 
	{
		data = YAML::LoadFile(filepath.string());
	} 
	catch (YAML::ParserException e) 
	{
		QK_CORE_LOGE_TAG("Scene", "Failed to load.qkscene file : {}", filepath.string());
		return false;
	}

	if (! data["Scene"]) 
	{
		QK_CORE_LOGE_TAG("Scene", "Scene file: {} does not contain Scene key", filepath.string());
		return false;
	}

	std::string sceneName = data["Scene"].as<std::string>();
	m_Scene->sceneName = sceneName;
	QK_CORE_LOGI_TAG("Scene", "Deserializing scene: {}", sceneName);

	auto entities = data["Entities"];
	if (entities)
	{
		for (auto entity : entities) 
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();

			std::string name;
			auto nameComponent = entity["NameComponent"];
			if (nameComponent)
				name = nameComponent.as<std::string>();
			QK_CORE_LOGT_TAG("Scene", "Deserializing entity with ID: {0} and name: {1}", uuid, name);

			Entity* deserializedEntity = m_Scene->CreateEntityWithID(uuid, name);

			auto transformCmpt = entity["TransformComponent"];
			if (transformCmpt)
			{
				// Entity always has transform component
				auto* tc = deserializedEntity->GetComponent<TransformCmpt>();
				tc->SetLocalPosition(transformCmpt["Position"].as<glm::vec3>());
				tc->SetLocalRotate(glm::quat(glm::radians(transformCmpt["Rotation"].as<glm::vec3>())));
				tc->SetLocalScale(transformCmpt["Scale"].as<glm::vec3>());
			}

			auto cameraCmpt = entity["CameraComponent"];
			if (cameraCmpt)
			{
				auto* cc = deserializedEntity->AddComponent<CameraCmpt>();
				cc->fov = cameraCmpt["Fov"].as<float>();
				cc->zNear = cameraCmpt["Near"].as<float>();
				cc->zFar = cameraCmpt["Far"].as<float>();
				cc->aspect = cameraCmpt["Aspect"].as<float>();
			}

			auto meshCmpt = entity["MeshComponent"];
			if (meshCmpt)
			{
				auto* mc = deserializedEntity->AddComponent<MeshCmpt>();
				uint64_t assetId = meshCmpt["AssetID"].as<uint64_t>();
				auto mesh = AssetManager::Get().GetAsset<Mesh>(assetId);
				
				mc->sharedMesh = mesh;
				mc->uniqueMesh = nullptr;
			}

			auto meshRendererCmpt = entity["MeshRendererComponent"];
			if (meshRendererCmpt)
			{
				auto* mrc = deserializedEntity->AddComponent<MeshRendererCmpt>();
				auto* mc = deserializedEntity->GetComponent<MeshCmpt>();
				QK_CORE_ASSERT(mc)

				Ref<Mesh> mesh = mc->uniqueMesh ? mc->uniqueMesh : mc->sharedMesh;
				mrc->SetMesh(mesh);

				auto materials = meshRendererCmpt["Materials"];
				uint32_t i = 0;
				for (auto mat : materials)
				{
					AssetID assetId = mat["AssetID"].as<AssetID>();

					if (assetId == 1)	// Default material
					{
						mrc->SetMaterial(i, AssetManager::Get().defaultMaterial);
					}
					else
					{
						auto material = AssetManager::Get().GetAsset<Material>(assetId);
						QK_CORE_ASSERT(material)
						mrc->SetMaterial(i, material);
					}

					i++;
				}
				QK_CORE_ASSERT(i == mesh->subMeshes.size())
			}
		}

		// Loop agein to establish parent-child relationship
		for (auto entity : entities)
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();
			Entity* deserializedEntity = m_Scene->GetEntityWithID(uuid);
			auto* relationshipCmpt = deserializedEntity->GetComponent<RelationshipCmpt>();

			auto children = entity["Children"];
			if (children)
			{
				for (auto child : children) 
				{
					uint64_t childId = child["Id"].as<uint64_t>();
					relationshipCmpt->AddChildEntity(m_Scene->GetEntityWithID(childId));
				}
			}
		}
	}

	return true;
}


bool SceneSerializer::DeserializeBinary(AssetID scene)
{
	// Not implemented
	QK_CORE_ASSERT(false);
	return false;
}

}