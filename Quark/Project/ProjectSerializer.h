#pragma once
#include "Quark/Project/Project.h"

namespace quark
{
	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project);

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
	private:
		Ref<Project> m_project;
	};
}
