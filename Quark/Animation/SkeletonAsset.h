#pragma once
#include "Quark/Asset/Asset.h"

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace quark 
{
	struct SkeletonAsset : public Asset
	{
		QUARK_ASSET_TYPE_DECL(SKELETON)

		static const uint32_t null_index = ~0;

		std::string skeleton_name;
		std::vector<std::string> bone_names;
		std::vector<uint32_t> parent_bone_indices;
		uint32_t root_bone_index = null_index;

		// rest pose of skeleton. All in bone-local space (i.e. translation/rotation/scale relative to parent)
		std::vector<glm::vec3> bone_translations;
		std::vector<glm::quat> bone_rotations;
		std::vector<glm::vec3> bone_scales;
		std::vector<glm::mat4> inverse_bind_matrices;

		// the skeleton itself can have a transform
		// notably this happens if the whole "armature" is rotated or scaled in DCC tool
		glm::mat4 skeleton_transform;

		uint32_t AddBone(const std::string& name, uint32_t parent_index, glm::mat4 inverse_bind_matrix);
		uint32_t GetBoneIndex(const std::string_view name) const;
		std::vector<uint32_t> GetChildBoneIndices(uint32_t parent_index) const;

	};
}