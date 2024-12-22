#pragma once
#include "Quark/Asset/ImageAsset.h"

namespace quark {

class ImageImporter {
public:
    ImageImporter() = default;

    Ref<ImageAsset> Import(const std::string& file_path);
    Ref<ImageAsset> ImportKtx(const std::string& file_path, bool isCubemap = false); // isCubemap isn't acutally needed, just making API more explicit
    Ref<ImageAsset> ImportKtx2(const std::string& file_path, bool isCubemap = false);
    Ref<ImageAsset> ImportStb(const std::string& file_path);

};

}