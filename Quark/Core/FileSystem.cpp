#include "Quark/QuarkPch.h"
#include "Quark/Core/FileSystem.h"

namespace quark {

bool FileSystem::Exists(const std::filesystem::path& filepath)
{
    return std::filesystem::exists(filepath);
}

bool FileSystem::Exists(const std::string& filepath)
{
    return std::filesystem::exists(std::filesystem::path(filepath));
}

bool FileSystem::ReadFile(const std::string& fileName, std::vector<byte>& data, size_t readSize, size_t offset)
{
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        size_t dataSize = (size_t)file.tellg() - offset;
        dataSize = std::min(dataSize, readSize);
        file.seekg((std::streampos)offset);
        data.resize(dataSize);
        file.read((char*)data.data(), dataSize);
        file.close();
        return true;
    }

    CORE_LOGW("FileSystem::ReadFile: Failed to open file {}", fileName);
    return false;
}

}