#include "Quark/qkpch.h"
#include "Quark/Core/Base.h"
#include "Quark/Project/ProjectSerializer.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace quark {
	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_project(project)
	{

	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "Project" << YAML::Value;
			{
				out << YAML::BeginMap;// Project
				out << YAML::Key << "Name" << YAML::Value << m_project->GetProjectName();
				out << YAML::Key << "AssetDirectory" << YAML::Value << m_project->GetAssetDirectory().string();
				out << YAML::Key << "AssetRegistry" << YAML::Value << m_project->GetAssetRegistryPath().string();
				out << YAML::Key << "StartScene" << YAML::Value << m_project->GetStartScenePath().string();
				out << YAML::EndMap; // Project
			}
			out << YAML::EndMap; // Root
		}
		std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			QK_CORE_LOGW_TAG("ProjectSerializer", "Failed to load project file {}", filepath.string(), e.what());
			return false;
		}

		auto projectNode = data["Project"];
		if (!projectNode)
			return false;

		m_project->m_name = projectNode["Name"].as<std::string>();
		m_project->m_startScenePath = projectNode["StartScene"].as<std::string>();
		m_project->m_assetDirectory = projectNode["AssetDirectory"].as<std::string>();

		m_project->m_projectDirectory = filepath.parent_path();
		m_project->m_projectFileName = filepath.filename().string();

		return true;
	}
}