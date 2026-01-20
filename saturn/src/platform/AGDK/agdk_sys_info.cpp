#include "agdk_sys_info.hpp"

#include <unistd.h>
#include <sys/statvfs.h>

namespace saturn
{
namespace platform
{
namespace agdk
{

std::unordered_map<std::string, std::string> AGDKSystemInfo::parseProcInfoFile(
    const std::string& _filePath)
{
    std::ifstream cpuInfoFile(_filePath);
    std::stringstream buffer;
    buffer << cpuInfoFile.rdbuf();
    std::string cpuInfoStr = buffer.str();

    std::unordered_map<std::string, std::string> entries;
    std::regex entryRegex(R"(([^:\n]+):[ \t]*([^\n]+))");
    auto entriesBegin = std::sregex_iterator(cpuInfoStr.begin(), cpuInfoStr.end(), entryRegex);
    auto entriesEnd = std::sregex_iterator();
    for (std::sregex_iterator i = entriesBegin; i != entriesEnd; ++i)
    {
        auto key = (*i)[1].str();
        key.erase(remove(key.begin(), key.end(), '\t'), key.end());
        auto value = (*i)[2].str();

        entries[key] = value;
    }

    return entries;
}

core::OSInfo AGDKSystemInfo::getOSInfo()
{
    core::OSInfo osInfo{};

    // Needs jni_on_load_generator.py to support fields

    return osInfo;
}

core::CPUInfo AGDKSystemInfo::getCPUInfo()
{
    core::CPUInfo info{};

    auto cpuInfoEntries = parseProcInfoFile("/proc/cpuinfo");

    info.model = cpuInfoEntries.at("model name");
    info.vendor = cpuInfoEntries.at("vendor_id");
    info.clockSpeedGHz = std::stod(cpuInfoEntries.at("cpu MHz")) / 1000.0f;
    info.physicalCores = std::stoi(cpuInfoEntries.at("cpu cores"));
    // No hyper-threading on Android, so logical cores are equal to physical cores
    info.logicalCores = info.physicalCores;

    std::string isaFlags = cpuInfoEntries.at("flags");

    if (isaFlags.find("neon") != std::string::npos) info.isaSupport.neon = true;
    if (isaFlags.find("sse") != std::string::npos) info.isaSupport.sse = true;
    if (isaFlags.find("sse2") != std::string::npos) info.isaSupport.sse2 = true;
    if (isaFlags.find("sse3") != std::string::npos) info.isaSupport.sse3 = true;
    if (isaFlags.find("sse4_1") != std::string::npos) info.isaSupport.sse41 = true;
    if (isaFlags.find("sse4_2") != std::string::npos) info.isaSupport.sse42 = true;

    // We can confidently say that mobile CPUs do not support any AVX versions

    return info;
}

core::MemoryMetrics AGDKSystemInfo::getMemoryMetrics()
{
    core::MemoryMetrics metrics{};

    auto memInfoEntries = parseProcInfoFile("/proc/meminfo");

    metrics.totalMemoryKB = std::stoull(memInfoEntries.at("MemTotal")) * 1024; // Convert kB to B
    metrics.freeMemoryKB = std::stoull(memInfoEntries.at("MemFree")) * 1024;   // Convert kB to B
    metrics.usedMemoryKB = metrics.totalMemoryKB - metrics.freeMemoryKB;

    return metrics;
}

std::vector<core::StorageDeviceInfo> AGDKSystemInfo::getStorageDevices()
{
    std::vector<core::StorageDeviceInfo> devicesInfo;

    // We don't support multiple storage devices on Android, so we will just return the internal
    // storage

    core::StorageDeviceInfo deviceInfo;

    deviceInfo.mountPoint = "/";
    deviceInfo.name = "Internal Storage";
    deviceInfo.type = core::StorageDeviceInfo::StorageType::SSD; // Assuming internal storage is SSD

    struct statvfs stat{};
    if (statvfs(deviceInfo.mountPoint.c_str(), &stat) == 0)
    {
        size_t blockSize = stat.f_frsize;
        size_t totalBlocks = stat.f_blocks;
        size_t availBlocks = stat.f_bavail;

        deviceInfo.metrics.totalStorageKB = (blockSize * totalBlocks) / 1024;
        deviceInfo.metrics.freeStorageKB = (blockSize * availBlocks) / 1024;
        deviceInfo.metrics.usedStorageKB =
            deviceInfo.metrics.totalStorageKB - deviceInfo.metrics.freeStorageKB;
    }

    devicesInfo.push_back(deviceInfo);

    return devicesInfo;
}

core::LocaleInfo AGDKSystemInfo::getLocaleInfo()
{
    core::LocaleInfo info;

    // Needs ICU4C to access locale information on Android

    return info;
}

std::vector<core::MonitorInfo> AGDKSystemInfo::getMonitors()
{
    std::vector<core::MonitorInfo> monitorsInfo;

    // We don't have multiple monitors on Android, so we will just return the primary monitor

    core::MonitorInfo primaryMonitor;

    // For now we leave the primary monitor's properties empty

    monitorsInfo.push_back(primaryMonitor);

    return monitorsInfo;
}

} // namespace agdk
} // namespace platform
} // namespace saturn
