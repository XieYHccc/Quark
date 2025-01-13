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

    QK_FORCE_INLINE float ElapsedSeconds()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f;
    }
    

    QK_FORCE_INLINE float ElapsedMillis()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

}