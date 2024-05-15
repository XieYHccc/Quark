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

#define CORE_ASSERT(expr)                                                 \
    {                                                                        \
        if (expr) {                                                          \
        } else {                                                             \
            CORE_LOGC("Assertion Failure: {}, in file: {}, line: {}", \
                #expr, __FILE__, __LINE__)                                   \
            debugBreak();                                                    \
        }                                                                    \
    }

#define CORE_ASSERT_MSG(expr, message)                                       \
    {                                                                           \
        if (expr) {                                                             \
        } else {                                                                \
            CORE_LOGC("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                  \
            debugBreak();                                                       \
        }                                                                       \
    }

#define CLIENT_ASSERT(expr)                                                \
    {                                                                         \
        if (expr) {                                                           \
        } else {                                                              \
            CLIENT_LOGC("Assertion Failure: {}, in file: {}, line: {}",\
                #expr, __FILE__, __LINE__)                                   \
            debugBreak();                                                     \
        }                                                                     \
    }

#define CLIENT_ASSERT_MSG(expr, message)                                       \
    {                                                                             \
        if (expr) {                                                               \
        } else {                                                                  \
            CLIENT_LOGC("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                                               \
            debugBreak();                                                         \
        }                                                                         \
    }
#ifndef NDEBUG
    #define CORE_DEBUG_ASSERT(expr) UE_CORE_ASSERT(expr)
    #define CORE_DEBUG_ASSERT_MSG(expr, message) UE_CORE_ASSERT(expr, message)
    #define CORE_DEBUG_ASSERT(expr) UE_CORE_ASSERT(expr)
    #define CORE_DEBUG_ASSERT_MSG(expr, message) UE_CORE_ASSERT(expr, message)
#else
    #define CORE_DEBUG_ASSERT(expr) 
    #define CLIENT_DEBUG_ASSERT(expr) 
    #define CORE_DEBUG_ASSERT_MSG(expr, message)
    #define CLIENT_DEBUG_ASSERT_MSG(expr, message) 
#endif

