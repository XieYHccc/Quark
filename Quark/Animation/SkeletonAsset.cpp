#include "Quark/qkpch.h"
#include "Quark/Animation/SkeletonAsset.h"

namespace quark
{
	uint32_t SkeletonAsset::AddBone(const std::string& name, uint32_t parent_index, glm::mat4 inverse_bind_matrix)
	{
		uint32_t index = static_cast<uint32_t>(bone_names.size());
		bone_names.emplace_back(name);
		parent_bone_indices.emplace_back(parent_index);
		inverse_bind_matrices.emplace_back(inverse_bind_matrix);

		return index;
	}

	uint32_t SkeletonAsset::GetBoneIndex(const std::string_view name) const
	{
		for (uint32_t i = 0; i < bone_names.size(); i++)
		{
			if (bone_names[i] == name)
			{
				return static_cast<uint32_t>(i);
			}
		}

		return null_index;
	}
	std::vector<uint32_t> SkeletonAsset::GetChildBoneIndices(uint32_t parent_index) const
	{
		std::vector<uint32_t> child_bone_indexes;
		for (size_t i = 0; i < parent_bone_indices.size(); ++i)
		{
			if (parent_bone_indices[i] == parent_index)
			{
				child_bone_indexes.emplace_back(static_cast<uint32_t>(i));
			}
		}
		return child_bone_indexes;
	}
}