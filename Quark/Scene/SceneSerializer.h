#pragma once
#include <filesystem>

#include "Quark/Asset/Asset.h"
#include "Quark/Scene/Scene.h"

namespace quark {
class SceneSerializer
{
public:
	SceneSerializer(const Ref<Scene>& scene);

	void Serialize(const std::filesystem::path& filepath);
	void SerializeBinary(AssetID scene);

	bool Deserialize(const std::filesystem::path& filepath);
	bool DeserializeBinary(AssetID scene);

public:
	inline static std::string_view FileFilter = "Quark Scene (*.qkscene)\0*.qkscene\0";
	inline static std::string_view DefaultExtension = ".qkscene";

private:
	Ref<Scene> m_Scene;
};
}