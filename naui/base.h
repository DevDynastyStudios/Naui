#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define NAUI_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
#define NAUI_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define NAUI_MACOS 1
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