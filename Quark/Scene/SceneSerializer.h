#pragma once
#include "Quark/Asset/Asset.h"

#include <filesystem>

namespace quark {
class Scene;
class SceneSerializer
{
public:
	SceneSerializer(Ref<Scene>& scene);

	void Serialize(const std::filesystem::path& filepath);
	void SerializeBinary(AssetID scene);

	bool Deserialize(const std::filesystem::path& filepath);
	bool DeserializeBinary(AssetID scene);

public:
	inline static std::string_view s_FileFilter = "Quark Scene (*.qkscene)\0*.qkscene\0";
	inline static std::string_view s_DefaultExtension = ".qkscene";

private:
	Ref<Scene> m_Scene;
};
}