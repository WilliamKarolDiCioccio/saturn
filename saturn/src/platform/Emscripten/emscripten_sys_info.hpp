#pragma once

#include "saturn/core/sys_info.hpp"

#include <emscripten.h>

namespace saturn
{
namespace platform
{
namespace emscripten
{

class EmscriptenSystemInfo : public core::SystemInfo::SystemInfoImpl
{
   public:
    virtual ~EmscriptenSystemInfo() = default;

    core::OSInfo getOSInfo() override;
    core::CPUInfo getCPUInfo() override;
    core::MemoryMetrics getMemoryMetrics() override;
    std::vector<core::StorageDeviceInfo> getStorageDevices() override;
    core::LocaleInfo getLocaleInfo() override;
    std::vector<core::MonitorInfo> getMonitors() override;
};

} // namespace emscripten
} // namespace platform
} // namespace saturn
