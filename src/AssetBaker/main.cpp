#include <iostream>
#include <fstream>
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "glm/glm.hpp"
#include <lz4.h>
#include <chrono>
#include <json.hpp>
#include <tiny_obj_loader.h>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <stb_image.h>

#include <IO.h>
#include <Texture.h>

namespace fs = std::filesystem;
using namespace assetlib;

struct ConverterState {
	fs::path asset_path;
	fs::path export_path;

	fs::path convert_to_export_relative(fs::path path) const;
};

bool convertImage(const fs::path& input, const fs::path& output)
{
	int texWidth, texHeight, texChannels;

	auto pngstart = std::chrono::high_resolution_clock::now();

	stbi_uc* pixels = stbi_load(input.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	auto pngend = std::chrono::high_resolution_clock::now();

	auto diff = pngend - pngstart;

	std::cout << "png took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms"  << std::endl;

	if (!pixels) {
		std::cout << "Failed to load texture file " << input << std::endl;
		return false;
	}
	
	int texture_size = texWidth * texHeight * 4;

	TextureInfo texinfo;
	texinfo.textureSize = texture_size;
	
	texinfo.textureFormat = TextureFormat::RGBA8;
	texinfo.originalFile = input.string();
	auto start = std::chrono::high_resolution_clock::now();

	std::vector<char> all_buffer;

	struct DumbHandler : nvtt::OutputHandler {
		// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
		virtual bool writeData(const void* data, int size) {
			for (int i = 0; i < size; i++) {
				buffer.push_back(((char*)data)[i]);
			}
			return true;
		}
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) { };

		// Indicate the end of the compressed image. (New in NVTT 2.1)
		virtual void endImage() {};
		std::vector<char> buffer;
	};


	
	nvtt::Compressor compressor;

	nvtt::CompressionOptions optiuns;
	nvtt::OutputOptions outputOptions;
	nvtt::Surface surface;

	DumbHandler handler;
	outputOptions.setOutputHandler(&handler);

	surface.setImage(nvtt::InputFormat::InputFormat_BGRA_8UB, texWidth, texHeight, 1, pixels);


	while (surface.canMakeNextMipmap(1))
	{
			surface.buildNextMipmap(nvtt::MipmapFilter_Box);

			optiuns.setFormat(nvtt::Format::Format_RGBA);
			optiuns.setPixelType(nvtt::PixelType_UnsignedNorm);

			compressor.compress(surface, 0, 0, optiuns, outputOptions);

			texinfo.pages.push_back({});
			texinfo.pages.back().width = surface.width();
			texinfo.pages.back().height = surface.height();
			texinfo.pages.back().originalSize = handler.buffer.size();

			all_buffer.insert(all_buffer.end(), handler.buffer.begin(), handler.buffer.end());
			handler.buffer.clear();
	}
	

	texinfo.textureSize = all_buffer.size();
	assets::AssetFile newImage = assets::pack_texture(&texinfo, all_buffer.data());

	auto  end = std::chrono::high_resolution_clock::now();



	std::cout << "compression took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms" << std::endl;
		

	stbi_image_free(pixels);

	save_binaryfile(output.string().c_str(), newImage);

	return true;
}





