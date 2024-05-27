#pragma once
#include "Util/Singleton.h"
#include "Rendering/RenderDeviceDriver.h"

class RenderDevice final : public MakeSingleton<RenderDevice>{
    template<typename >
    friend class MakeSingleton;
public:
    bool Init();
    void ShutDown();

    bool BeiginFrame(f32 deltaTime);
    bool EndFrame(f32 deltaTime);

    void Resize(u16 width, u16 height);

    // Events callbacks
    void OnWindowResize();

private:
    RenderDevice() {}
    ~RenderDevice() {}
    
    RenderDeviceDriver* m_Backend;
};