#include "Quark/QuarkPch.h"
#include "Quark/Scene/SceneSerializer.h"

#include <yaml-cpp/yaml.h>

#include "Quark/Core/Application.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"

namespace YAML {
template<>
struct convert<glm::vec3>
{
	static Node encode(const glm::vec3& rhs)
	{
		Node node;
		node.push_back(rhs.x);
		node.push_back(rhs.y);
		node.push_back(rhs.z);
		node.SetStyle(EmitterStyle::Flow);
		return node;
	}

	static bool decode(const Node& node, glm::vec3& rhs)
	{
		if (!node.IsSequence() || node.size() != 3)
			return false;

		rhs.x = node[0].as<float>();
		rhs.y = node[1].as<float>();
		rhs.z = node[2].as<float>();
		return true;
	}
};

}

namespace quark {
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
	return out;
}

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
		out << YAML::Key << "Position" << YAML::Value << transformCmpt->GetPosition();
		out << YAML::Key << "Rotation" << YAML::Value << glm::degrees(glm::eulerAngles(transformCmpt->GetQuat()));
		out << YAML::Key << "Scale" << YAML::Value << transformCmpt->GetScale();

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
		
	out << YAML::EndMap; // Entity
}

SceneSerializer::SceneSerializer(Scene* scene)
	: m_Scene(scene)
{

}

void SceneSerializer::Serialize(const std::filesystem::path& filepath)
{

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetSceneName();
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
	CORE_ASSERT(false);
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
		CORE_LOGE("Failed to load .qkscene file: {}", filepath.string())
		return false;
	}

	if (! data["Scene"]) 
	{
		CORE_LOGE("Scene file: {} does not contain Scene key", filepath.string())
		return false;
	}

	std::string sceneName = data["Scene"].as<std::string>();
	m_Scene->SetSceneName(sceneName);
	CORE_LOGD("Deserializing scene: {}", sceneName);

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
			CORE_LOGI("Deserializing entity with ID: {0} and name: {1}", uuid, name);

			Entity* deserializedEntity = m_Scene->CreateEntityWithID(uuid, name);

			auto transformCmpt = entity["TransformComponent"];
			if (transformCmpt)
			{
				// Entity always has transform component
				auto* tc = deserializedEntity->GetComponent<TransformCmpt>();
				tc->SetPosition(transformCmpt["Position"].as<glm::vec3>());
				tc->SetQuat(glm::quat(glm::radians(transformCmpt["Rotation"].as<glm::vec3>())));
				tc->SetScale(transformCmpt["Scale"].as<glm::vec3>());
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
	CORE_ASSERT(false);
	return false;
}

}