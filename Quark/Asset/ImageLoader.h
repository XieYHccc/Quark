#pragma once
#include "Graphic/Device.h"

namespace asset {

class ImageLoader {
public:
    ImageLoader(graphic::Device* device);

    Ref<graphic::Image> LoadKtx(const std::string& file_path);
    
private:
    graphic::Device* graphicDevice_;
};

}