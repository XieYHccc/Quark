#pragma once
#include "Quark/Asset/Asset.h"

#include <glm/glm.hpp>

namespace quark
{
	struct AnimationSampler
	{
		std::string            interpolation;
		std::vector<float>     inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct AnimationChannel
	{
		std::string path;
		uint32_t boneIndex;
		uint32_t samplerIndex;
	};

	struct AnimationAsset : public Asset
	{
		QUARK_ASSET_TYPE_DECL(ANIMATION)

		std::string                   name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float                         start = std::numeric_limits<float>::max();
		float                         end = std::numeric_limits<float>::min();
		float                         currentTime = 0.0f;
	};

}