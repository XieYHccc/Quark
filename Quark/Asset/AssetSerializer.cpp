#include "Quark/qkpch.h"
#include "Quark/Asset/AssetSerializer.h"
#include "Quark/Core/Util/SerializationUtils.h"

#include <yaml-cpp/yaml.h>

namespace quark {

void MaterialSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
{

}

bool MaterialSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
{
	return false;
}

std::string MaterialSerializer::SerializeToYAML(Ref<Material> materialAsset) const
{
	YAML::Emitter out;
	out << YAML::BeginMap; // Material
	out << YAML::Key << "Material" << YAML::Value;
	{
		out << YAML::BeginMap;

	}

	out << YAML::EndMap;

	return std::string(out.c_str());
}

bool MaterialSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<Material>& targetMaterialAsset, AssetID id) const
{
	return false;
}

}