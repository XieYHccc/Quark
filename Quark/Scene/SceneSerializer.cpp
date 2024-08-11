#include "Quark/QuarkPch.h"
#include "Quark/Scene/SceneSerializer.h"

#include <yaml-cpp/yaml.h>
namespace quark {

static void SerializeGameObject(YAML::Emitter& out, Entity* entity)
{
	
}

SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
	: m_Scene(scene)
{

}

void SceneSerializer::Serialize(const std::filesystem::path& filepath)
{

}

void SceneSerializer::SerializeBinary(AssetID scene)
{
	// Not implemented
	CORE_ASSERT(false);
}

bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
{
	return false;
}


bool SceneSerializer::DeserializeBinary(AssetID scene)
{
	// Not implemented
	CORE_ASSERT(false);
	return false;
}

}