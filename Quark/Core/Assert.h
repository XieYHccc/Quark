#pragma once
#include "Quark/Core/Logger.h"

namespace quark {
    namespace internal 
    {
        /// Function called when an assertion fails. This function should return true if a breakpoint needs to be triggered
        QK_FORCE_INLINE void AssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
        {
            // Log the error
            CORE_LOGC("Assertion Failed: {} in file: {}, line: {}", inExpression, inFile, inLine);
            if (inMessage)
                CORE_LOGC("Message: {}", inMessage);

            QK_DEBUGBREAK();
        }

        // Helper functions to pass message on to failed function
        struct AssertLastParam { };
        QK_FORCE_INLINE void AssertFailedParamHelper(const char* inExpression, const char* inFile, uint32_t inLine, AssertLastParam) { AssertFailed(inExpression, nullptr, inFile, inLine); }
        QK_FORCE_INLINE void AssertFailedParamHelper(const char* inExpression, const char* inFile, uint32_t inLine, const char* inMessage, AssertLastParam) { AssertFailed(inExpression, inMessage, inFile, inLine); }
    }

#define QK_ASSERT(expr, ...) do { if (!expr) internal::AssertFailedParamHelper(#expr, __FILE__, __LINE__, ##__VA_ARGS__, internal::AssertLastParam()); } while (false)


/////////////////////////// Old API ///////////////////////////
///////////////////////////////////////////////////////////////
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
    #define CORE_DEBUG_ASSERT_MSG(expr, message) CORE_ASSERT_MSG(expr, message)
    #define DEBUG_ASSERT(expr) ASSERT(expr)
#else
    #define CORE_DEBUG_ASSERT(expr) 
    #define CORE_DEBUG_ASSERT_MSG(expr, message) 
    #define DEBUG_ASSERT(expr) 
#endif

}