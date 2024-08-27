#pragma once
#include "Quark/Asset/Texture.h"

namespace quark {

class TextureImporter {
public:
    TextureImporter() = default;

    Ref<Texture> ImportKtx(const std::string& file_path, bool isCubemap = false); // isCubemap isn't acutally needed,just making API more explicit
    Ref<Texture> ImportKtx2(const std::string& file_path, bool isCubemap = false);
    Ref<Texture> ImportStb(const std::string& file_path);


};

}