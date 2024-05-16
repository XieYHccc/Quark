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

#define ASSERT(expr)                                                \
    {                                                                         \
        if (expr) {                                                           \
        } else {                                                              \
            CLIENT_LOGC("Assertion Failure: {}, in file: {}, line: {}",\
                #expr, __FILE__, __LINE__)                                   \
            debugBreak();                                                     \
        }                                                                     \
    }

#define ASSERT_MSG(expr, message)                                       \
    {                                                                             \
        if (expr) {                                                               \
        } else {                                                                  \
            LOGC("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                                               \
            debugBreak();                                                         \
        }                                                                         \
    }
#ifndef NDEBUG
    #define CORE_DEBUG_ASSERT(expr) CORE_ASSERT(expr)
    #define CORE_DEBUG_ASSERT_MSG(expr, message) CORE_ASSERT_MSG(expr, message)
    #define DEBUG_ASSERT(expr) ASSERT(expr)
    #define DEBUG_ASSERT_MSG(expr, message) ASSERT_MSG(expr, message)
#else
    #define CORE_DEBUG_ASSERT(expr) 
    #define DEBUG_ASSERT(expr) 
    #define CORE_DEBUG_ASSERT_MSG(expr, message)
    #define DEBUG_ASSERT_MSG(expr, message) 
#endif

