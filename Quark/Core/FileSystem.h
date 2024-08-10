#pragma once
#include <string>
#include <filesystem>

namespace quark {

class FileSystem {
public:
	static bool Exists(const std::filesystem::path& filepath);
	static bool Exists(const std::string& filepath);
	static bool ReadFile(const std::string& fileName, std::vector<byte>& data, size_t readSize = ~0ull, size_t offset = 0);
};
}