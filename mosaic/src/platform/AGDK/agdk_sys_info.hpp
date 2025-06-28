#pragma once

#include "mosaic/core/sys_info.hpp"

#include <fstream>
#include <regex>

#include "mosaic/platform/AGDK/jni_helper.hpp"
#include "mosaic/platform/AGDK/agdk_platform.hpp"

namespace mosaic
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
} // namespace mosaic
