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

    f64 Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
    }

    f64 ElapsedMillis()
    {
        return Elapsed() * 1000.0f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};