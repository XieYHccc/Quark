#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Graphic/Device.h"

namespace quark {
class GpuResourceManager : public util::MakeSingleton<GpuResourceManager> {
public:
    // Default resources
    inline static Ref<graphic::Image> whiteImage;
    inline static Ref<graphic::Image> blackImage;
    inline static Ref<graphic::Image> checkboardImage;

    inline static Ref<graphic::Sampler> linearSampler;
    inline static Ref<graphic::Sampler> nearestSampler;
    inline static Ref<graphic::Sampler> cubeMapSampler;

    GpuResourceManager() = default;
    ~GpuResourceManager() = default;

    void Init();
    void Shutdown();


};
}