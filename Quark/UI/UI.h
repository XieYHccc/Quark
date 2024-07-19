#pragma once
#include "Util/Singleton.h"
#include "Graphic/Device.h"

class UI : public util::MakeSingletonPtr<UI>{
public:
    enum WindowFlagBit {

    };
    using WindowFlags = int;
    
    UI() = default;
    virtual ~UI() = default;

    virtual void Init(graphic::Device* device) = 0;
    virtual void Finalize() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Render(graphic::CommandList* cmd) = 0;

    virtual bool BeginBlock(const char* name, WindowFlags flags = 0) = 0;
    virtual void EndBlock() = 0;
    virtual void Text(const char* formatstr, ...) = 0;
};

template <>
template <>
UI* util::MakeSingletonPtr<UI>::CreateSingleton();