#pragma once

#define NAUI_PLATFORM_WINDOWS 0
#define NAUI_PLATFORM_LINUX 0
#define NAUI_PLATFORM_MAC 0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define NAUI_PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
    #define NAUI_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define NAUI_PLATFORM_MAC 1
#endif

#ifdef NAUI_EXPORT
    #ifdef _MSC_VER
        #define NAUI_API __declspec(dllexport)
    #else
        #define NAUI_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _MSC_VER
        #define NAUI_API __declspec(dllimport)
    #else
        #define NAUI_API
    #endif
#endif

#define NAUI_COMPILER_MSVC   0
#define NAUI_COMPILER_CLANG  0
#define NAUI_COMPILER_GCC    0
#define NAUI_COMPILER_UNKNOWN 0

#if defined(_MSC_VER)
    #define NAUI_COMPILER_MSVC 1
#elif defined(__clang__)
    #define NAUI_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define NAUI_COMPILER_GCC 1
#else
    #define NAUI_COMPILER_UNKNOWN 1
#endif

#if NAUI_COMPILER_MSVC
    #define NAUI_FORCE_INLINE __forceinline
    #define NAUI_NO_INLINE __declspec(noinline)
#elif NAUI_COMPILER_GCC || NAUI_COMPILER_CLANG
    #define NAUI_FORCE_INLINE inline __attribute__((always_inline))
    #define NAUI_NO_INLINE __attribute__((noinline))
#else
    #define NAUI_FORCE_INLINE inline
    #define NAUI_NO_INLINE
#endif

#if NAUI_COMPILER_MSVC
    #define NAUI_ALIGN(x) __declspec(align(x))
#elif NAUI_COMPILER_GCC || NAUI_COMPILER_CLANG
    #define NAUI_ALIGN(x) __attribute__((aligned(x)))
#else
    #define NAUI_ALIGN(x)
#endif

#if NAUI_COMPILER_GCC || NAUI_COMPILER_CLANG
    #define NAUI_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define NAUI_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define NAUI_LIKELY(x)   (x)
    #define NAUI_UNLIKELY(x) (x)
#endif

#if NAUI_COMPILER_MSVC
    #define NAUI_RESTRICT __restrict
#elif NAUI_COMPILER_GCC || NAUI_COMPILER_CLANG
    #define NAUI_RESTRICT __restrict__
#else
    #define NAUI_RESTRICT
#endif