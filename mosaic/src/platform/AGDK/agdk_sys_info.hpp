#pragma once

#include "saturn/core/sys_info.hpp"

#include <fstream>
#include <regex>

#include "saturn/platform/AGDK/jni_helper.hpp"
#include "saturn/platform/AGDK/agdk_platform.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

class AGDKSystemInfo : public core::SystemInfo::SystemInfoImpl
{
   private:
    std::unordered_map<std::string, std::string> parseProcInfoFile(const std::string& _filePath);

   public:
    virtual ~AGDKSystemInfo() = default;

    core::OSInfo getOSInfo() override;
    core::CPUInfo getCPUInfo() override;
    core::MemoryMetrics getMemoryMetrics() override;
    std::vector<core::StorageDeviceInfo> getStorageDevices() override;
    core::LocaleInfo getLocaleInfo() override;
    std::vector<core::MonitorInfo> getMonitors() override;
};

} // namespace agdk
} // namespace platform
} // namespace saturn
