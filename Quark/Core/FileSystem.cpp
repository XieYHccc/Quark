#include "Quark/qkpch.h"
#include <nfd.hpp>

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

std::filesystem::path FileSystem::OpenFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters)
{
    NFD::UniquePath filePath;
    nfdresult_t result = NFD::OpenDialog(filePath, (const nfdfilteritem_t*)inFilters.begin(), inFilters.size());

    switch (result)
    {
    case NFD_OKAY: return filePath.get();
    case NFD_CANCEL: return "";
    case NFD_ERROR:
    {
        CORE_LOGE("NFD-Extended threw an error: {}", NFD::GetError());
        return "";
    }
    }
}

std::filesystem::path FileSystem::SaveFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters)
{
    NFD::UniquePath filePath;
    nfdresult_t result = NFD::SaveDialog(filePath, (const nfdfilteritem_t*)inFilters.begin(), inFilters.size());

    switch (result)
    {
    case NFD_OKAY: return filePath.get();
    case NFD_CANCEL: return "";
    case NFD_ERROR:
    {
        CORE_LOGE("NFD-Extended threw an error: {}", NFD::GetError());
        return "";
    }
    }
}

}