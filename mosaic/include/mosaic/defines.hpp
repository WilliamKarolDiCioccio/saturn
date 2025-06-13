#pragma once

// Platform detection
#if defined(__EMSCRIPTEN__)
#define MOSAIC_PLATFORM_EMSCRIPTEN
#define MOSCAIC_PLATFORM_NAME "Emscripten"
#elif defined(__ANDROID__) || defined(ANDROID)
#define MOSAIC_PLATFORM_ANDROID
#define MOSCAIC_PLATFORM_NAME "Android"
#elif defined(_WIN32) || defined(_WIN64)
#define MOSAIC_PLATFORM_WINDOWS
#define MOSCAIC_PLATFORM_NAME "Windows"
#elif defined(__APPLE__) || defined(__MACH__)
#define MOSAIC_PLATFORM_MACOS
#define MOSCAIC_PLATFORM_NAME "macOS"
#elif defined(__linux__)
#define MOSAIC_PLATFORM_LINUX
#define MOSCAIC_PLATFORM_NAME "Linux"
#else
#error "Unknown platform!"
#endif

// Platform type detection
#if defined(MOSAIC_PLATFORM_WINDOWS) || defined(MOSAIC_PLATFORM_MACOS) || \
    defined(MOSAIC_PLATFORM_LINUX)
#define MOSAIC_PLATFORM_DESKTOP
#define MOSAIC_PLATFORM_TYPE "Desktop"
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
#define MOSAIC_PLATFORM_WEB
#define MOSAIC_PLATFORM_TYPE "Web"
#elif defined(MOSAIC_PLATFORM_ANDROID)
#define MOSAIC_PLATFORM_MOBILE
#define MOSAIC_PLATFORM_TYPE "Mobile"
#endif

// Compiler detection
#if defined(__EMSCRIPTEN__)
#define MOSAIC_COMPILER_EMSCRIPTEN
#define MOSAIC_COMPILER_NAME "Emscripten"
#elif defined(_MSC_VER)
#define MOSAIC_COMPILER_MSVC
#define MOSAIC_COMPILER_NAME "MSVC"
#elif defined(__clang__)
#define MOSAIC_COMPILER_CLANG
#define MOSAIC_COMPILER_NAME "Clang"
#elif defined(__GNUC__) || defined(__GNUG__)
#define MOSAIC_COMPILER_GCC
#define MOSAIC_COMPILER_NAME "GCC"
#else
#error "Unknown compiler!"
#endif

// Import/Export macros
#if defined(MOSAIC_COMPILER_MSVC)
#if defined(_MOSAIC_BUILD_EXPORT_DLL)
#define MOSAIC_API __declspec(dllexport)
#else
#define MOSAIC_API __declspec(dllimport)
#endif
#elif defined(MOSAIC_COMPILER_GCC) || defined(MOSAIC_COMPILER_CLANG)
#if defined(_MOSAIC_BUILD_EXPORT_DLL)
#define MOSAIC_API __attribute__((visibility("default")))
#else
#define MOSAIC_API
#endif
#else
#define MOSAIC_API
#endif

// Debug break macros
#ifdef _DEBUG
#if defined(MOSAIC_COMPILER_MSVC)
#define MOSAIC_DEBUGBREAK __debugbreak()
#elif defined(MOSAIC_COMPILER_GCC) || defined(MOSAIC_COMPILER_CLANG)
#define MOSAIC_DEBUGBREAK raise(SIGTRAP)
#endif
#else
#define MOSAIC_DEBUGBREAK ((void)0)
#endif

// Assert macros
#ifdef _DEBUG
#define MOSAIC_ASSERT_IMPL(cond, file, func, line, msg) \
    do                                                  \
    {                                                   \
        if (!(cond))                                    \
        {                                               \
            const char* _assert_condition = #cond;      \
            const char* _assert_file = file;            \
            const char* _assert_func = func;            \
            int _assert_line = line;                    \
            const char* _assert_msg = msg;              \
            /* Optional: Call a logger or breakpoint */ \
            DEBUG_BREAK();                              \
            std::abort();                               \
        }                                               \
    } while (false)
#define MOSAIC_ASSERT(x) MOSAIC_ASSERT_IMPL(x, __FILE__, __FUNCTION__, __LINE__, nullptr)
#define MOSAIC_ASSERT_MSG(x, msg) MOSAIC_ASSERT_IMPL(x, __FILE__, __FUNCTION__, __LINE__, msg)
#else
#define MOSAIC_ASSERT(x) ((void)0)
#define MOSAIC_ASSERT_MSG(x, msg) ((void)0)
#endif

// Warning suppression macros
#if defined(MOSAIC_COMPILER_EMSCRIPTEN)
#define MOSAIC_PUSH_WARNINGS
#define MOSAIC_POP_WARNINGS
#define MOSAIC_DISABLE_ALL_WARNINGS
#define MOSAIC_DISABLE_WARNING(warning)
#elif defined(MOSAIC_COMPILER_MSVC)
#define MOSAIC_PUSH_WARNINGS __pragma(warning(push))
#define MOSAIC_POP_WARNINGS __pragma(warning(pop))
#define MOSAIC_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
#define MOSAIC_DISABLE_WARNING(warning) __pragma(warning(disable : warning))
#elif defined(MOSAIC_COMPILER_GCC) || defined(MOSAIC_COMPILER_CLANG)
#define MOSAIC_PUSH_WARNINGS _Pragma("GCC diagnostic push")
#define MOSAIC_POP_WARNINGS _Pragma("GCC diagnostic pop")
#define MOSAIC_DISABLE_ALL_WARNINGS                                            \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wall\"") \
        _Pragma("GCC diagnostic ignored \"-Wextra\"")                          \
            _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define MOSAIC_DISABLE_WARNING(warning) _Pragma(MOSAIC_STRINGIFY(GCC diagnostic ignored warning))
#endif

// Function signature
#if defined(MOSAIC_COMPILER_MSVC)
#define MOSAIC_FUNC_SIGNATURE __FUNCSIG__
#elif defined(MOSAIC_COMPILER_GCC) || defined(MOSAIC_COMPILER_CLANG)
#define MOSAIC_FUNC_SIGNATURE __PRETTY_FUNCTION__
#else
#define MOSAIC_FUNC_SIGNATURE __func__
#endif

// Helper macro for string conversion
#define MOSAIC_STRINGIFY_HELPER(x) #x
#define MOSAIC_STRINGIFY(x) MOSAIC_STRINGIFY_HELPER(x)

// SI (decimal) unit definitions
#define BYTES_PER_KB 1000UL
#define BYTES_PER_MB 1000000UL
#define BYTES_PER_GB 1000000000UL

// IEC (binary) unit definitions
#define BYTES_PER_KIB 1024UL
#define BYTES_PER_MIB 1048576UL    /* 1024^2 */
#define BYTES_PER_GIB 1073741824UL /* 1024^3 */

// SI (decimal) conversion macros - Bytes to larger units
#define BYTES_TO_KB(bytes) ((bytes) / BYTES_PER_KB)
#define BYTES_TO_MB(bytes) ((bytes) / BYTES_PER_MB)
#define BYTES_TO_GB(bytes) ((bytes) / BYTES_PER_GB)

// SI (decimal) conversion macros - Larger units to bytes
#define KB_TO_BYTES(kb) ((kb) * BYTES_PER_KB)
#define MB_TO_BYTES(mb) ((mb) * BYTES_PER_MB)
#define GB_TO_BYTES(gb) ((gb) * BYTES_PER_GB)

// IEC (binary) conversion macros - Bytes to larger units
#define BYTES_TO_KIB(bytes) ((bytes) / BYTES_PER_KIB)
#define BYTES_TO_MIB(bytes) ((bytes) / BYTES_PER_MIB)
#define BYTES_TO_GIB(bytes) ((bytes) / BYTES_PER_GIB)

// IEC (binary) conversion macros - Larger units to bytes
#define KIB_TO_BYTES(kib) ((kib) * BYTES_PER_KIB)
#define MIB_TO_BYTES(mib) ((mib) * BYTES_PER_MIB)
#define GIB_TO_BYTES(gib) ((gib) * BYTES_PER_GIB)

// Ceiling division macros (round up) - Bytes to larger units
#define BYTES_TO_KB_CEIL(bytes) (((bytes) + BYTES_PER_KB - 1) / BYTES_PER_KB)
#define BYTES_TO_MB_CEIL(bytes) (((bytes) + BYTES_PER_MB - 1) / BYTES_PER_MB)
#define BYTES_TO_GB_CEIL(bytes) (((bytes) + BYTES_PER_GB - 1) / BYTES_PER_GB)
#define BYTES_TO_KIB_CEIL(bytes) (((bytes) + BYTES_PER_KIB - 1) / BYTES_PER_KIB)
#define BYTES_TO_MIB_CEIL(bytes) (((bytes) + BYTES_PER_MIB - 1) / BYTES_PER_MIB)
#define BYTES_TO_GIB_CEIL(bytes) (((bytes) + BYTES_PER_GIB - 1) / BYTES_PER_GIB)

// Remainder macros - Get remainder bytes after conversion
#define BYTES_REMAINDER_KB(bytes) ((bytes) % BYTES_PER_KB)
#define BYTES_REMAINDER_MB(bytes) ((bytes) % BYTES_PER_MB)
#define BYTES_REMAINDER_GB(bytes) ((bytes) % BYTES_PER_GB)
#define BYTES_REMAINDER_KIB(bytes) ((bytes) % BYTES_PER_KIB)
#define BYTES_REMAINDER_MIB(bytes) ((bytes) % BYTES_PER_MIB)
#define BYTES_REMAINDER_GIB(bytes) ((bytes) % BYTES_PER_GIB)

// Conversion between units - SI (decimal)
#define KB_TO_MB(kb) ((kb) / BYTES_PER_KB)
#define KB_TO_GB(kb) ((kb) / (BYTES_PER_GB / BYTES_PER_KB))
#define MB_TO_KB(mb) ((mb) * (BYTES_PER_MB / BYTES_PER_KB))
#define MB_TO_GB(mb) ((mb) / (BYTES_PER_GB / BYTES_PER_MB))
#define GB_TO_KB(gb) ((gb) * (BYTES_PER_GB / BYTES_PER_KB))
#define GB_TO_MB(gb) ((gb) * (BYTES_PER_GB / BYTES_PER_MB))

// Conversion between units - IEC (binary)
#define KIB_TO_MIB(kib) ((kib) / BYTES_PER_KIB)
#define KIB_TO_GIB(kib) ((kib) / (BYTES_PER_GIB / BYTES_PER_KIB))
#define MIB_TO_KIB(mib) ((mib) * (BYTES_PER_MIB / BYTES_PER_KIB))
#define MIB_TO_GIB(mib) ((mib) / (BYTES_PER_GIB / BYTES_PER_MIB))
#define GIB_TO_KIB(gib) ((gib) * (BYTES_PER_GIB / BYTES_PER_KIB))
#define GIB_TO_MIB(gib) ((gib) * (BYTES_PER_GIB / BYTES_PER_MIB))
