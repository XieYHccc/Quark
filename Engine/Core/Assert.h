#pragma once
#include "Core/Base.h"

#define CORE_ASSERT(expr)                                                 \
    {                                                                        \
        if (expr) {                                                          \
        } else {                                                             \
            CORE_LOGC("Assertion Failure: {}, in file: {}, line: {}", \
                #expr, __FILE__, __LINE__)                                   \
            QK_DEBUGBREAK();                                                    \
        }                                                                    \
    }

#define CORE_ASSERT_MSG(expr, message)                                       \
    {                                                                           \
        if (expr) {                                                             \
        } else {                                                                \
            CORE_LOGC("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                  \
            QK_DEBUGBREAK();                                                       \
        }                                                                       \
    }

#define ASSERT(expr)                                                \
    {                                                                         \
        if (expr) {                                                           \
        } else {                                                              \
            LOGC("Assertion Failure: {}, in file: {}, line: {}",\
                #expr, __FILE__, __LINE__)                                   \
            QK_DEBUGBREAK();                                                     \
        }                                                                     \
    }

#define ASSERT_MSG(expr, message)                                       \
    {                                                                             \
        if (expr) {                                                               \
        } else {                                                                  \
            LOGC("Assertion Failure: {}, message: {}, in file: {}," \
                "line: {}", #expr, message,  __FILE__, __LINE__)                                               \
            QK_DEBUGBREAK();                                                         \
        }                                                                         \
    }
#ifdef QK_DEBUG_BUILD
    #define CORE_DEBUG_ASSERT(expr) CORE_ASSERT(expr)
    #define DEBUG_ASSERT(expr) ASSERT(expr)
#else
    #define CORE_DEBUG_ASSERT(expr) 
    #define DEBUG_ASSERT(expr) 
#endif

