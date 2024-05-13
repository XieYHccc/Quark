#pragma once
#include "Defines.h"
#include "Core/Logger.h"

// Causes a debug breakpoint to be hit.
#if _MSC_VER
    #include <intrin.h>
    #define debugBreak() __debugbreak()
#else
    #define debugBreak() __builtin_trap()
#endif

#define UE_CORE_ASSERT(expr)                                                 \
    {                                                                        \
        if (expr) {                                                          \
        } else {                                                             \
            XE_CORE_CRITICAL("Assertion Failure: {}, in file: {}, line: {}", \
                #expr, __FILE__, __LINE__)                                   \
            debugBreak();                                                    \
        }                                                                    \
    }

#define UE_CORE_ASSERT_MSG(expr, message)                                       \
    {                                                                           \
        if (expr) {                                                             \
        } else {                                                                \
            UE_CORE_CRITICAL("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                  \
            debugBreak();                                                       \
        }                                                                       \
    }

#define UE_CLIENT_ASSERT(expr)                                                \
    {                                                                         \
        if (expr) {                                                           \
        } else {                                                              \
            UE_CLIENT_CRITICAL("Assertion Failure: {}, in file: {}, line: {}",\
                #expr, __FILE__, __LINE__)                                   \
            debugBreak();                                                     \
        }                                                                     \
    }

#define UE_CLIENT_ASSERT_MSG(expr, message)                                       \
    {                                                                             \
        if (expr) {                                                               \
        } else {                                                                  \
            UE_CLIENT_CRITICAL("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                                               \
            debugBreak();                                                         \
        }                                                                         \
    }
#ifndef NDEBUG
    #define UE_CORE_DEBUG_ASSERT(expr) UE_CORE_ASSERT(expr)
    #define UE_CORE_DEBUG_ASSERT_MSG(expr, message) UE_CORE_ASSERT(expr, message)
    #define UE_CORE_DEBUG_ASSERT(expr) UE_CORE_ASSERT(expr)
    #define UE_CORE_DEBUG_ASSERT_MSG(expr, message) UE_CORE_ASSERT(expr, message)
#else
    #define UE_CORE_DEBUG_ASSERT(expr) 
    #define UE_CLIENT_DEBUG_ASSERT(expr) 
    #define UE_CORE_DEBUG_ASSERT_MSG(expr, message)
    #define UE_CLIENT_DEBUG_ASSERT_MSG(expr, message) 
#endif

