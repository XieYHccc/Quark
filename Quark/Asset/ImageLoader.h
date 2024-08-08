#pragma once
#include "Quark/Graphic/Device.h"

namespace quark {

class ImageLoader {
public:
    ImageLoader(graphic::Device* device) : graphicDevice_(device) {}

    Ref<graphic::Image> LoadKtx(const std::string& file_path);
    Ref<graphic::Image> LoadKtx2(const std::string& file_path);
    
private:
    graphic::Device* graphicDevice_;
};

}