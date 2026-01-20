#pragma once

#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "saturn/defines.hpp"

namespace saturn
{
namespace core
{

// Represents metrics related to storage devices in the system
struct StorageMetrics
{
    size_t totalStorageKB; // Total storage in kilobytes
    size_t usedStorageKB;  // Used storage in kilobytes
    size_t freeStorageKB;  // Free storage in kilobytes

    StorageMetrics() : totalStorageKB(0), usedStorageKB(0), freeStorageKB(0) {};
};

// Represents a storage device in the system
struct StorageDeviceInfo
{
    std::string mountPoint; // Mount point (e.g., "/mnt/c", "C:\\")
    std::string name;       // Device name (e.g., "C:", "/dev/sda1")

    enum class StorageType
    {
        HDD,    // Hard Disk Drive
        SSD,    // Solid State Drive
        USB,    // USB Drive
        Network // Network Attached Storage (NAS)
    } type;     // Type of storage (HDD, SSD, USB, Network)

    StorageMetrics metrics; // Storage metrics for the device

    StorageDeviceInfo() : mountPoint("."), name("Unknown"), type(StorageType::HDD), metrics() {};
};

// Represents memory metrics of the system
struct MemoryMetrics
{
    size_t totalMemoryKB; // Total memory in kilobytes
    size_t usedMemoryKB;  // Used memory in kilobytes
    size_t freeMemoryKB;  // Free memory in kilobytes

    MemoryMetrics() : totalMemoryKB(0), usedMemoryKB(0), freeMemoryKB(0) {};
};

struct CPUInfo
{
    std::string model;        // CPU model (e.g., "Intel Core i7-9700K")
    std::string vendor;       // CPU vendor (e.g., "Intel", "AMD")
    std::string architecture; // CPU architecture (e.g., "x86_64", "arm64")
    uint32_t cacheSizeKB;     // CPU cache size in kilobytes
    uint32_t logicalCores;    // Number of logical cores (threads)
    uint32_t physicalCores;   // Number of physical cores
    float clockSpeedGHz;      // Clock speed in GHz

    struct ISASupport
    {
        bool sse;     // Streaming SIMD Extensions
        bool sse2;    // Streaming SIMD Extensions 2
        bool sse3;    // Streaming SIMD Extensions 3
        bool ssse3;   // Supplemental Streaming SIMD Extensions 3
        bool sse41;   // Streaming SIMD Extensions 4.1
        bool sse42;   // Streaming SIMD Extensions 4.2
        bool avx;     // Advanced Vector Extensions
        bool avx2;    // Advanced Vector Extensions 2
        bool avx512;  // Advanced Vector Extensions 512
        bool avx10;   // Advanced Vector Extensions 10
        bool avx10_2; // Advanced Vector Extensions 10.2
        bool neon;    // ARM Neon support
        bool apx;     // Advanced Pixel Extensions
        bool fp16;    // Floating Point 16-bit support

        ISASupport()
            : sse(false),
              sse2(false),
              sse3(false),
              ssse3(false),
              sse41(false),
              sse42(false),
              avx(false),
              avx2(false),
              avx512(false),
              avx10(false),
              avx10_2(false),
              neon(false),
              apx(false),
              fp16(false) {};
    } isaSupport;

    CPUInfo()
        : model("Unknown"),
          vendor("Unknown"),
          architecture("Unknown"),
          cacheSizeKB(0),
          logicalCores(0),
          physicalCores(0),
          clockSpeedGHz(0.0f),
          isaSupport() {};
};

struct MonitorInfo
{
    std::string name;  // Full display name (e.g., "Dell UltraSharp U2415")
    std::string model; // Model identifier (e.g., "U2415")

    glm::ivec2 resolution; // Pixel dimensions (e.g., 1920x1080)
    double refreshRate;    // Refresh rate in Hz

    glm::vec2 aspectRatio; // Aspect ratio (computed from resolution)
    glm::vec2 dpi;         // Dots per inch (DPI) for X and Y axes
    glm::ivec2 position;   // Position in virtual desktop space

    bool isPrimary;      // Is this the primary display?
    bool isConnected;    // Currently connected?
    bool isEnabled;      // Currently enabled (not sleeping, powered on)?
    bool isHDR;          // Supports High Dynamic Range (HDR)?
    bool isTouchEnabled; // Supports touch input?

    enum class Orientation
    {
        Landscape,
        Portrait,
        LandscapeFlipped,
        PortraitFlipped
    } orientation;

    enum class ColorDepth : uint8_t
    {
        Bit8 = 8,
        Bit10 = 10,
        Bit12 = 12,
        Bit16 = 16
    } colorDepth;

    enum class ColorSpace
    {
        sRGB,
        DCI_P3,
        Adobe_RGB,
        Rec2020,
        Wide_Gamut
    } colorSpace;

    MonitorInfo()
        : name("Unknown"),
          model("Unknown"),
          resolution(0, 0),
          refreshRate(60.0),
          aspectRatio(16.0f / 9.0f, 1.0f),
          dpi(96.0f, 96.0f),
          position(0, 0),
          isPrimary(false),
          isConnected(true),
          isEnabled(true),
          isHDR(false),
          isTouchEnabled(false),
          orientation(Orientation::Landscape),
          colorDepth(ColorDepth::Bit8),
          colorSpace(ColorSpace::sRGB) {};
};

struct OSInfo
{
    std::string name;          // Operating system name (e.g., "Windows", "Linux")
    std::string version;       // Operating system version (e.g., "10.0.19042")
    std::string kernelVersion; // Kernel version (e.g., "5.4.0-42-generic")
    std::string build;         // OS build number (e.g., "19042")

    OSInfo() : name("Unknown"), version("Unknown"), kernelVersion("Unknown"), build("Unknown") {};
};

struct LocaleInfo
{
    std::string locale;       // e.g., "en_US.UTF-8"
    std::string languageCode; // ISO 639-1 (e.g., "en")
    std::string countryCode;  // ISO 3166-1 alpha-2 (e.g., "US")
    std::string encoding;     // e.g., "UTF-8"

    std::string timezone;          // e.g., "Europe/Rome"
    int32_t timezoneOffsetMinutes; // Offset in minutes from UTC (e.g., +120)

    std::string currencyCode;   // ISO 4217 (e.g., "USD", "EUR")
    std::string currencySymbol; // e.g., "$", "€"

    std::string shortDateFormat; // e.g., "MM/DD/YYYY"
    std::string longDateFormat;  // e.g., "MMMM DD, YYYY"
    std::string timeFormat;      // e.g., "HH:mm:ss"
    std::string dateTimeFormat;  // e.g., "YYYY-MM-DD HH:mm:ss"

    char decimalSeparator;   // e.g., '.'
    char thousandsSeparator; // e.g., ','

    enum class Weekday
    {
        Sunday,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday
    } firstDayOfWeek;

    LocaleInfo()
        : locale("en_US.UTF-8"),
          languageCode("en"),
          countryCode("US"),
          encoding("UTF-8"),
          timezone("UTC"),
          timezoneOffsetMinutes(0),
          currencyCode("USD"),
          currencySymbol("$"),
          shortDateFormat("MM/DD/YYYY"),
          longDateFormat("MMMM DD, YYYY"),
          timeFormat("HH:mm:ss"),
          dateTimeFormat("YYYY-MM-DD HH:mm:ss"),
          decimalSeparator('.'),
          thousandsSeparator(','),
          firstDayOfWeek(Weekday::Monday) {};
};

/**
 * @brief Provides system information, such as OS, CPU, memory, storage, locale, and monitors
 * properties and metrics.
 */
class SystemInfo
{
   public:
    class SystemInfoImpl
    {
       public:
        SystemInfoImpl() = default;
        virtual ~SystemInfoImpl() = default;

       public:
        virtual OSInfo getOSInfo() = 0;
        virtual CPUInfo getCPUInfo() = 0;
        virtual MemoryMetrics getMemoryMetrics() = 0;
        virtual std::vector<StorageDeviceInfo> getStorageDevices() = 0;
        virtual LocaleInfo getLocaleInfo() = 0;
        virtual std::vector<MonitorInfo> getMonitors() = 0;
    };

   private:
    SATURN_API static std::unique_ptr<SystemInfoImpl> impl;

   public:
    SystemInfo(const SystemInfo&) = delete;
    SystemInfo& operator=(const SystemInfo&) = delete;
    SystemInfo(SystemInfo&&) = delete;
    SystemInfo& operator=(SystemInfo&&) = delete;

   public:
    SATURN_API [[nodiscard]] static OSInfo getOSInfo();
    SATURN_API [[nodiscard]] static CPUInfo getCPUInfo();
    SATURN_API [[nodiscard]] static MemoryMetrics getMemoryMetrics();
    SATURN_API [[nodiscard]] static std::vector<StorageDeviceInfo> getStorageDevices();
    SATURN_API [[nodiscard]] static LocaleInfo getLocaleInfo();
    SATURN_API [[nodiscard]] static std::vector<MonitorInfo> getMonitors();
};

} // namespace core
} // namespace saturn
