#include "Asset/ImageLoader.h"

namespace asset {
Ref<graphic::Image> ImageLoader::LoadKtx(const std::string& file_path) {
    if (file_path.find_last_of(".") != std::string::npos) {
        if (file_path.substr(file_path.find_last_of(".") + 1) != "ktx") {
            CORE_LOGW("ImageLoader::LoadKtx: The file {} is not a ktx file", file_path);
            return nullptr;
        }
    }
    

}

}