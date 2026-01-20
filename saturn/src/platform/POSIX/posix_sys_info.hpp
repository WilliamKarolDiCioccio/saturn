#pragma once

#include "saturn/core/sys_info.hpp"

#include <memory>

namespace saturn
{
namespace platform
{
namespace posix
{

class POSIXSystemInfo : public core::SystemInfo::SystemInfoImpl
{
   public:
    POSIXSystemInfo() = default;
    ~POSIXSystemInfo() override = default;

    core::OSInfo getOSInfo() override;
    core::CPUInfo getCPUInfo() override;
    core::MemoryMetrics getMemoryMetrics() override;
    std::vector<core::StorageDeviceInfo> getStorageDevices() override;
    core::LocaleInfo getLocaleInfo() override;
    std::vector<core::MonitorInfo> getMonitors() override;
};

} // namespace posix
} // namespace platform
} // namespace saturn
