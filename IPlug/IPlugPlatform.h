#pragma once

/**
 * @file
 * @brief Include to get consistently named preprocessor macros for different platforms and logging functionality
 */

#ifdef _WIN32
  #define OS_WIN
#elif defined __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_MAC
    #define OS_OSX
  #endif
#elif defined __linux || defined __linux__ || defined linux
  #define OS_LINUX
#elif defined EMSCRIPTEN
  #define OS_WEB
#else
  #error "No OS defined!"
#endif

#if defined(_WIN64) || defined(__LP64__)
  #define ARCH_64BIT 
#endif

#include "IPlugLogger.h"

