#pragma once
#include "Util/Singleton.h"
#include "Graphic/Device.h"

class UI : public util::MakeSingletonPtr<UI>{
public:

    UI() = default;
    virtual ~UI() = default;

    virtual void Init(graphic::Device* device) = 0;
    virtual void Finalize() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Render(graphic::CommandList* cmd) = 0;
};

template <>
template <>
UI* util::MakeSingletonPtr<UI>::CreateSingleton();