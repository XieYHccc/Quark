#pragma once

#include <chrono>

#include "Core/Base.h"

class Timer
{
public:
    Timer()
    {
        Reset();
    }

    void Reset()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    QK_FORCE_INLINE f64 Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }
    
    QK_FORCE_INLINE f64 ElapsedMicros()
    {
        return Elapsed() / 1000.0f;
    }

    QK_FORCE_INLINE f64 ElapsedMillis()
    {
        return ElapsedMicros() / 1000.0f;
    }

    QK_FORCE_INLINE f64 ElapsedSeconds()
    {
        return ElapsedMillis() / 1000.0f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};