#include "posix_sys_info.hpp"

namespace saturn
{
namespace platform
{
namespace posix
{

core::OSInfo POSIXSystemInfo::getOSInfo()
{
    core::OSInfo osInfo{};

    return osInfo;
}

core::CPUInfo POSIXSystemInfo::getCPUInfo()
{
    core::CPUInfo info{};

    return info;
}

core::MemoryMetrics POSIXSystemInfo::getMemoryMetrics()
{
    core::MemoryMetrics metrics{};

    return metrics;
}

std::vector<core::StorageDeviceInfo> POSIXSystemInfo::getStorageDevices()
{
    std::vector<core::StorageDeviceInfo> devicesInfo;

    return devicesInfo;
}

core::LocaleInfo POSIXSystemInfo::getLocaleInfo()
{
    core::LocaleInfo info;

    return info;
}

std::vector<core::MonitorInfo> POSIXSystemInfo::getMonitors()
{
    std::vector<core::MonitorInfo> monitorsInfo;

    return monitorsInfo;
}

} // namespace posix
} // namespace platform
} // namespace saturn
