#include "Util/FileSystem.h"

namespace util {

bool FileRead(const std::string& fileName, std::vector<uint8_t>& data, size_t readSize, size_t offset)
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

    CORE_LOGW("util::FileRead: Failed to open file {}", fileName);
    return false;
}

}