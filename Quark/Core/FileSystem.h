#pragma once
#include <string>
#include <filesystem>

#include "Quark/Core/Base.h"

namespace quark {
class FileSystem {
public:
	static bool Exists(const std::filesystem::path& filepath);
	static bool Exists(const std::string& filepath);
	static bool ReadFile(const std::string& fileName, std::vector<byte>& data, size_t readSize = ~0ull, size_t offset = 0);

	static uint64_t GetLastWriteTime(const std::filesystem::path& filepath);
};
}