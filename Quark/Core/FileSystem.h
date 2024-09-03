#pragma once
#include <string>
#include <filesystem>

#include "Quark/Core/Base.h"

namespace quark {
class FileSystem {
public:
	static bool Exists(const std::filesystem::path& filepath);
	static bool Exists(const std::string& filepath);

	static uint64_t GetLastWriteTime(const std::filesystem::path& filepath);

	static bool ReadFileBinary(const std::string& fileName, std::vector<byte>& data, size_t readSize = ~0ull, size_t offset = 0);
	static bool WriteFileBinary(const std::string& fileName, const std::vector<byte>& data);
	static bool ReadFileText(const std::string& fileName, std::string& outString);

public:
	struct FileDialogFilterItem
	{
		const char* name;
		const char* spec;
	};

	static std::filesystem::path OpenFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters = {});
	static std::filesystem::path SaveFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters = {});
};
}