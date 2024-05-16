#pragma once

// Properly define static assertions.
#define STATIC_ASSERT static_assert

// Unsigned int types.
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

// Signed int types.
using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

// Floating point types
using f32 = float;
using f64 = double;

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");
STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");
STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");
STATIC_ASSERT(sizeof(bool) == 1, "Expected bool to be 1 byte.");

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define PLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif
#elif defined(__linux__) || defined(__gnu_linux__)
    #define PLATFORM_LINUX 1
    #if defined(__ANDROID__)
        #define PLATFORM_ANDROID 1
    #endif
#elif defined(__APPLE__)
    #define PLATFORM_APPLE 1
#else
    #error "Unknown platform!"
#endif

// Import/export qualifier
#ifdef KEXPORT
// Exports
    #ifdef _MSC_VER
        #define QK_API __declspec(dllexport)
    #else
        #define QK_API __attribute__((visibility("default")))
    #endif
#else
// Imports
    #ifdef _MSC_VER
        #define QK_API __declspec(dllimport)
    #else
        #define QK_API
    #endif
#endif

// Inlining
#if defined(__clang__) || defined(__gcc__)
    #define QK_FORCE_INLINE __attribute__((always_inline)) inline
    #define QK_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define QK_FORCE_INLINE __forceinline
    #define QK_NOINLINE __declspec(noinline)
#endif
