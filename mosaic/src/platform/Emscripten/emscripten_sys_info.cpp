#include "emscripten_sys_info.hpp"

namespace saturn
{
namespace platform
{
namespace emscripten
{

core::OSInfo EmscriptenSystemInfo::getOSInfo()
{
    core::OSInfo info;

    // We can consider the user agent information as the OS information for Emscripten
    info.name = EM_ASM_INT({ return navigator.userAgent; });
    info.version = EM_ASM_INT({ return navigator.platform; });
    info.kernelVersion = EM_ASM_INT({ return navigator.appVersion; });
    // We cannot determine a build number in browser environments
    info.build = "Unknown";

    return info;
}

core::CPUInfo EmscriptenSystemInfo::getCPUInfo()
{
    core::CPUInfo info;

    // Emscripten does not provide detailed CPU information like other platforms.

    return info;
}

core::MemoryMetrics EmscriptenSystemInfo::getMemoryMetrics()
{
    core::MemoryMetrics metrics;

    // Emscripten does not provide direct access to memory metrics like other platforms.

    return metrics;
}

std::vector<core::StorageDeviceInfo> EmscriptenSystemInfo::getStorageDevices()
{
    std::vector<core::StorageDeviceInfo> devices;

    // Emscripten does not support storage devices in the same way as other platforms.
    // Resources are typically downloaded and stored in memory or indexedDB.
    // Therefore, we will simulate a single virtual storage device.

    core::StorageDeviceInfo device;

    device.mountPoint = "/";
    device.name = "Virtual Storage";
    device.type = core::StorageInfo::StorageType::Network;
    device.metrics.totalStorageKB = 0;
    device.metrics.freeStorageKB = 0;
    device.metrics.usedStorageKB = 0;

    devices.push_back(device);

    return devices;
}

core::LocaleInfo EmscriptenSystemInfo::getLocaleInfo()
{
    core::LocaleInfo info;

    // For now we leave the locale information empty

    return info;
}

std::vector<core::MonitorInfo> EmscriptenSystemInfo::getMonitors()
{
    std::vector<core::MonitorInfo> monitors;

    // We don't have multiple monitors on Emscripten, so we will just return the primary monitor

    core::MonitorInfo primaryMonitor;

    // For now we leave the primary monitor's properties empty

    return monitors;
}

} // namespace emscripten
} // namespace platform
} // namespace saturn
