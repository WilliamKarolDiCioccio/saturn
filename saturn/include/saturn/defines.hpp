#pragma once

#include <new>

// Platform detection
#if defined(__EMSCRIPTEN__)
#define SATURN_PLATFORM_EMSCRIPTEN
#define SATURN_PLATFORM_NAME "Emscripten"
#elif defined(__ANDROID__) || defined(ANDROID)
#define SATURN_PLATFORM_ANDROID
#define SATURN_PLATFORM_NAME "Android"
#elif defined(_WIN32) || defined(_WIN64)
#define SATURN_PLATFORM_WINDOWS
#define SATURN_PLATFORM_NAME "Windows"
#elif defined(__APPLE__) || defined(__MACH__)
#define SATURN_PLATFORM_MACOS
#define SATURN_PLATFORM_NAME "macOS"
#elif defined(__linux__)
#define SATURN_PLATFORM_LINUX
#define SATURN_PLATFORM_NAME "Linux"
#else
#error "Unknown platform!"
#endif

// Platform type detection
#if defined(SATURN_PLATFORM_WINDOWS) || defined(SATURN_PLATFORM_MACOS) || \
    defined(SATURN_PLATFORM_LINUX)
#define SATURN_PLATFORM_DESKTOP
#define SATURN_PLATFORM_TYPE "Desktop"
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
#define SATURN_PLATFORM_WEB
#define SATURN_PLATFORM_TYPE "Web"
#elif defined(SATURN_PLATFORM_ANDROID)
#define SATURN_PLATFORM_MOBILE
#define SATURN_PLATFORM_TYPE "Mobile"
#endif

// Compiler detection
#if defined(__EMSCRIPTEN__)
#define SATURN_COMPILER_EMSCRIPTEN
#define SATURN_COMPILER_NAME "Emscripten"
#elif defined(_MSC_VER)
#define SATURN_COMPILER_MSVC
#define SATURN_COMPILER_NAME "MSVC"
#elif defined(__clang__)
#define SATURN_COMPILER_CLANG
#define SATURN_COMPILER_NAME "Clang"
#elif defined(__GNUC__) || defined(__GNUG__)
#define SATURN_COMPILER_GCC
#define SATURN_COMPILER_NAME "GCC"
#else
#error "Unknown compiler!"
#endif

// Import/Export macros
#if defined(SATURN_COMPILER_MSVC)
#if defined(_SATURN_BUILD_EXPORT_DLL)
#define SATURN_API __declspec(dllexport)
#else
#define SATURN_API __declspec(dllimport)
#endif
#elif defined(SATURN_COMPILER_GCC) || defined(SATURN_COMPILER_CLANG)
#if defined(_SATURN_BUILD_EXPORT_DLL)
#define SATURN_API __attribute__((visibility("default")))
#else
#define SATURN_API
#endif
#else
#define SATURN_API
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define SATURN_ARCH_X64
#define SATURN_ARCH_NAME "x86_64"
#define SATURN_ARCH_BITS 64
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#define SATURN_ARCH_X86
#define SATURN_ARCH_NAME "x86"
#define SATURN_ARCH_BITS 32
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SATURN_ARCH_ARM64
#define SATURN_ARCH_NAME "ARM64"
#define SATURN_ARCH_BITS 64
#elif defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
#define SATURN_ARCH_ARM32
#define SATURN_ARCH_NAME "ARM"
#define SATURN_ARCH_BITS 32
#elif defined(__wasm64__) || defined(__wasm__)
#if defined(__wasm64__)
#define SATURN_ARCH_WASM64
#define SATURN_ARCH_NAME "WASM64"
#define SATURN_ARCH_BITS 64
#else
#define SATURN_ARCH_WASM32
#define SATURN_ARCH_NAME "WASM32"
#define SATURN_ARCH_BITS 32
#endif
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__)
#if defined(__mips64) || defined(__mips64_)
#define SATURN_ARCH_MIPS64
#define SATURN_ARCH_NAME "MIPS64"
#define SATURN_ARCH_BITS 64
#else
#define SATURN_ARCH_MIPS
#define SATURN_ARCH_NAME "MIPS"
#define SATURN_ARCH_BITS 32
#endif
#elif defined(__riscv) || defined(__riscv__)
#if defined(__riscv_xlen) && (__riscv_xlen == 64)
#define SATURN_ARCH_RISCV64
#define SATURN_ARCH_NAME "RISC-V64"
#define SATURN_ARCH_BITS 64
#else
#define SATURN_ARCH_RISCV32
#define SATURN_ARCH_NAME "RISC-V32"
#define SATURN_ARCH_BITS 32
#endif
#elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__)
#define SATURN_ARCH_PPC64
#define SATURN_ARCH_NAME "PowerPC64"
#define SATURN_ARCH_BITS 64
#elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
#define SATURN_ARCH_PPC
#define SATURN_ARCH_NAME "PowerPC"
#define SATURN_ARCH_BITS 32
#else
#define SATURN_ARCH_UNKNOWN
#define SATURN_ARCH_NAME "Unknown"
#define SATURN_ARCH_BITS 0
#endif

#if SATURN_ARCH_BITS == 64
#define SATURN_ARCH_64BIT
#else
#define SATURN_ARCH_32BIT
#endif

// Endianness hint (if available)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define SATURN_ARCH_LITTLE_ENDIAN
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SATURN_ARCH_BIG_ENDIAN
#endif
#endif

// Debug break macros
#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
#if defined(SATURN_COMPILER_MSVC)
#define SATURN_DEBUGBREAK __debugbreak()
#elif defined(SATURN_COMPILER_GCC) || defined(SATURN_COMPILER_CLANG)
#define SATURN_DEBUGBREAK raise(SIGTRAP)
#endif
#else
#define SATURN_DEBUGBREAK ((void)0)
#endif

// Assert macros
#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
#define SATURN_ASSERT_IMPL(cond, file, func, line, msg) \
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
            SATURN_DEBUGBREAK();                        \
            std::abort();                               \
        }                                               \
    } while (false)
#define SATURN_ASSERT(x) SATURN_ASSERT_IMPL(x, __FILE__, __FUNCTION__, __LINE__, nullptr)
#define SATURN_ASSERT_MSG(x, msg) SATURN_ASSERT_IMPL(x, __FILE__, __FUNCTION__, __LINE__, msg)
#else
#define SATURN_ASSERT(x) ((void)0)
#define SATURN_ASSERT_MSG(x, msg) ((void)0)
#endif

// Warning suppression macros
#if defined(SATURN_COMPILER_EMSCRIPTEN)
#define SATURN_PUSH_WARNINGS
#define SATURN_POP_WARNINGS
#define SATURN_DISABLE_ALL_WARNINGS
#define SATURN_DISABLE_WARNING(warning)
#elif defined(SATURN_COMPILER_MSVC)
#define SATURN_PUSH_WARNINGS __pragma(warning(push))
#define SATURN_POP_WARNINGS __pragma(warning(pop))
#define SATURN_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
#define SATURN_DISABLE_WARNING(warning) __pragma(warning(disable : warning))
#elif defined(SATURN_COMPILER_GCC) || defined(SATURN_COMPILER_CLANG)
#define SATURN_PUSH_WARNINGS _Pragma("GCC diagnostic push")
#define SATURN_POP_WARNINGS _Pragma("GCC diagnostic pop")
#define SATURN_DISABLE_ALL_WARNINGS                                            \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wall\"") \
        _Pragma("GCC diagnostic ignored \"-Wextra\"")                          \
            _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define SATURN_DISABLE_WARNING(warning) _Pragma(SATURN_STRINGIFY(GCC diagnostic ignored warning))
#endif

// Function signature
#if defined(SATURN_COMPILER_MSVC)
#define SATURN_FUNC_SIGNATURE __FUNCSIG__
#elif defined(SATURN_COMPILER_GCC) || defined(SATURN_COMPILER_CLANG)
#define SATURN_FUNC_SIGNATURE __PRETTY_FUNCTION__
#else
#define SATURN_FUNC_SIGNATURE __func__
#endif

// Helper macro for string conversion
#define SATURN_STRINGIFY_HELPER(x) #x
#define SATURN_STRINGIFY(x) SATURN_STRINGIFY_HELPER(x)

// Cache line size definition (assumed 64 bytes for most modern architectures, used to prevent
// false sharing in multithreaded code)

#if defined(__cpp_lib_hardware_interference_size)
static constexpr std::size_t k_cacheLine = std::hardware_destructive_interference_size;
#else
// fallback - define SATURN_CACHE_LINE_SIZE elsewhere or pick 64
static constexpr std::size_t k_cacheLine = 64;
#endif

#define SATURN_CACHE_LINE_SIZE k_cacheLine

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
