#pragma once
#include "Quark/Core/Base.h"

#include <chrono>

namespace quark {
class Timer {
public:
    Timer()
    {
        Reset();
    }

    void Reset()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    QK_FORCE_INLINE float Elapsed()
    {
        return (float)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }
    
    QK_FORCE_INLINE float ElapsedMicros()
    {
        return Elapsed() * 0.001f;
    }

    QK_FORCE_INLINE float ElapsedMillis()
    {
        return ElapsedMicros() * 0.001f;
    }

    QK_FORCE_INLINE float ElapsedSeconds()
    {
        return ElapsedMillis() * 0.001f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

}