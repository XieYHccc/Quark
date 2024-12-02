#include "Quark/qkpch.h"
#include "Project.h"

namespace quark {
    std::filesystem::path Project::GetProjectDirectory()
    {
        return m_projectDirectory;
    }

    std::filesystem::path Project::GetAssetDirectory()
    {
        return  m_projectDirectory / m_assetDirectory;
    }

    std::filesystem::path Project::GetAssetRegistryPath()
    {
        return m_projectDirectory / m_assetRegistry;
    }

    std::filesystem::path Project::GetStartScenePath()
    {
        return GetAssetDirectory() / m_startScenePath;
    }
}


