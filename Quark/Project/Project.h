#pragma once
#include "Quark/Core/Base.h"

#include <filesystem>

namespace quark
{
	class Project
	{
	public:
		Project() = default;

		std::filesystem::path GetProjectDirectory(); // absolute path
		std::filesystem::path GetAssetDirectory();	// relative to project directory
		std::filesystem::path GetAssetRegistryPath(); // relative to project directory
		std::filesystem::path GetStartScenePath();	// relative to asset directory

		const std::string& GetProjectName() const { return m_name; }
		const std::string& GetProjectFileName() const { return m_projectFileName; }

		static void SetActive(Ref<Project> proj) { s_activeProject = proj; }
		static Ref<Project> GetActive() { return s_activeProject; }

	private:
		// serialization
		std::string m_name = "Untitled";
		std::filesystem::path m_assetDirectory = "Assets";
		std::filesystem::path m_assetRegistry = "Assets/AssetRegistry.qkr";
		std::filesystem::path m_startScenePath;

		// not serialized
		std::filesystem::path m_projectDirectory;
		std::string m_projectFileName;

		inline static Ref<Project> s_activeProject;

		friend class ProjectSerializer;
	};
}