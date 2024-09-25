#pragma once
#include "Quark/Core/Logger.h"

// Causes a debug breakpoint to be hit.
#if defined(QK_PLATFORM_WINDOWS)
#include <intrin.h>
#define QK_DEBUG_BREAK __debugbreak()
#elif defined(QK_PLATFORM_MACOS)
#define QK_DEBUG_BREAK __builtin_trap()
#else
#error "Platform doesn't support debugbreak yet!"
#endif

#ifdef QK_DEBUG_BUILD
#define QK_ENABLE_ASSERTS
#endif

#define QK_ENABLE_VERIFY

#ifdef QK_ENABLE_ASSERTS
    #ifdef QK_COMPILER_CLANG
        #define QK_CORE_ASSERT_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CORE, "Assertion Failed", #condition, ##__VA_ARGS__)
        #define QK_ASSERT_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CLIENT, "Assertion Failed", #condition, ##__VA_ARGS__)
    #else
        #define QK_CORE_ASSERT_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CORE, "Assertion Failed", #condition, ##__VA_ARGS__)
        #define QK_ASSERT_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CLIENT, "Assertion Failed", #condition, ##__VA_ARGS__)
#endif

    #define QK_CORE_ASSERT(condition, ...) { if(!(condition)) { QK_CORE_ASSERT_MESSAGE_INTERNAL(condition, __VA_ARGS__); QK_DEBUG_BREAK; } }
    #define QK_APP_ASSERT(condition, ...) { if(!(condition)) { QK_ASSERT_MESSAGE_INTERNAL(condition, __VA_ARGS__); QK_DEBUG_BREAK; } }
#else
    #define QK_CORE_ASSERT(condition, ...)
    #define QK_APP_ASSERT(condition, ...)
#endif

#ifdef QK_ENABLE_VERIFY
    #ifdef QK_COMPILER_CLANG
        #define QK_CORE_VERIFY_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CORE, "Verify Failed", #condition, ##__VA_ARGS__)
        #define QK_VERIFY_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CLIENT, "Verify Failed", #condition, ##__VA_ARGS__)
    #else
        #define QK_CORE_VERIFY_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CORE, "Verify Failed", #condition, ##__VA_ARGS__)
        #define QK_VERIFY_MESSAGE_INTERNAL(condition, ...)  ::quark::Logger::PrintAssertMessage(::quark::Logger::Type::CLIENT, "Verify Failed",  #condition, ##__VA_ARGS__)
    #endif

    #define QK_CORE_VERIFY(condition, ...) { if(!(condition)) { QK_CORE_VERIFY_MESSAGE_INTERNAL(condition, __VA_ARGS__); QK_DEBUG_BREAK; } }
    #define QK_APP_VERIFY(condition, ...) { if(!(condition)) { QK_VERIFY_MESSAGE_INTERNAL(condition, __VA_ARGS__); QK_DEBUG_BREAK; } }
#else
    #define QK_CORE_VERIFY(condition, ...)
    #define QK_APP_VERIFY(condition, ...)
#endif
