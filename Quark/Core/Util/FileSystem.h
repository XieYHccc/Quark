#pragma once
#include <string>
namespace quark::util {

bool FileRead(const std::string& fileName, std::vector<uint8_t>& data, size_t readSize = ~0ull, size_t offset = 0);
}