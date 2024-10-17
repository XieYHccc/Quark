#include "Quark/qkpch.h"
#include "Quark/Core/FileSystem.h"

#include <nfd.hpp>

namespace quark {
std::filesystem::path FileSystem::GetWorkingDirectory()
{
    return std::filesystem::current_path();
}

void FileSystem::SetWorkingDirectory(std::filesystem::path path)
{
    std::filesystem::current_path(path);
}

bool FileSystem::CreateDirectory(const std::filesystem::path& directory)
{
    return std::filesystem::create_directories(directory);
}

bool FileSystem::CreateDirectory(const std::string& directory)
{
    return CreateDirectory(std::filesystem::path(directory));
}

bool FileSystem::Exists(const std::filesystem::path& filepath)
{
    return std::filesystem::exists(filepath);
}

bool FileSystem::Exists(const std::string& filepath)
{
    return std::filesystem::exists(std::filesystem::path(filepath));
}

bool FileSystem::ReadFileBytes(const std::string& fileName, std::vector<byte>& data, size_t readSize, size_t offset)
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

    QK_CORE_LOGW_TAG("Core", "FileSystem::ReadFile: Failed to open file{}", fileName);
    return false;
}

bool FileSystem::ReadFileText(const std::string& fileName, std::string& outString)
{
    std::vector<byte> data;
    if (!ReadFileBytes(fileName, data))
        return false;

    outString = std::string(data.begin(), data.end());

    // Remove DOS EOL.
    outString.erase(std::remove_if(std::begin(outString), std::end(outString), [](char c) { return c == '\r'; }), std::end(outString));
    return true;
}

std::string FileSystem::GetExtension(const std::string& filepath)
{
    auto index = filepath.find_last_of('.');
    if (index == std::string::npos)
        return "";
    else
        return filepath.substr(index + 1, std::string::npos);
}

std::string FileSystem::GetExtension(const std::filesystem::path& filepath)
{
    std::string pathString = filepath.string();
    return GetExtension(pathString);
}

std::filesystem::path FileSystem::OpenFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters)
{
    NFD::UniquePath filePath;
    nfdresult_t result = NFD::OpenDialog(filePath, (const nfdfilteritem_t*)inFilters.begin(), (nfdresult_t)inFilters.size());

    switch (result)
    {
    case NFD_OKAY: 
        return filePath.get();
        break;
    case NFD_CANCEL: 
        return "";
        break;
    case NFD_ERROR:
        QK_CORE_LOGE_TAG("Core", "NFD-Extended threw an error: {}", NFD::GetError());
        return "";
    default:
        QK_CORE_VERIFY(false);
        return "";
    }
}

std::filesystem::path FileSystem::SaveFileDialog(const std::initializer_list<FileDialogFilterItem> inFilters)
{
    NFD::UniquePath filePath;
    nfdresult_t result = NFD::SaveDialog(filePath, (const nfdfilteritem_t*)inFilters.begin(), inFilters.size());

    switch (result)
    {
    case NFD_OKAY:
        return filePath.get();
        break;
    case NFD_CANCEL:
        return "";
        break;
    case NFD_ERROR:
        QK_CORE_LOGE_TAG("Core", "NFD-Extended threw an error: {}", NFD::GetError());
        return "";
    default:
        QK_CORE_VERIFY(false);
        return "";
    }
}

}