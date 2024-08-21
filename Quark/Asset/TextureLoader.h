#pragma once
#include "Quark/Asset/Texture.h"

namespace quark {

class TextureLoader {
public:
    TextureLoader() = default;

    Ref<graphic::Image> LoadKtx(const std::string& file_path);
    Ref<Texture> LoadKtx2(const std::string& file_path, bool isCubemap = false);
    Ref<Texture> LoadStb(const std::string& file_path);


};

}