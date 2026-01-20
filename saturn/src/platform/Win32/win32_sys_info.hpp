#pragma once

#include "saturn/core/sys_info.hpp"

#include <memory>

#include <windows.h>
#include <sysinfoapi.h>
#include <winreg.h>
#include <wbemidl.h>
#include <comdef.h>

#include "saturn/platform/Win32/wstring.hpp"

namespace saturn
{
namespace platform
{
namespace win32
{

std::wstring StringToWString(const std::string& _str);
std::string WStringToString(const std::wstring& _wstr);

class Win32SystemInfo : public core::SystemInfo::SystemInfoImpl
{
   private:
    /**
     * This class provides a helper for querying WMI (Windows Management Instrumentation).
     *
     * @see https://learn.microsoft.com/en-us/windows/win32/wmisdk/wmi-start-page
     */
    class WMIHelper
    {
       private:
        IWbemLocator* m_pLoc;
        IWbemServices* m_pSvc;
        bool m_initialized;

       public:
        WMIHelper();
        ~WMIHelper();

        bool initialize();
        void shutdown();

        uint32_t queryWMIUInt32(const std::wstring& _query, const std::wstring& _property);
        uint64_t queryWMIUInt64(const std::wstring& _query, const std::wstring& _property);
        std::string queryWMIString(const std::wstring& _query, const std::wstring& _property);
        std::vector<std::string> queryWMIStringArray(const std::wstring& _query,
                                                     const std::wstring& _property);
    };

    std::unique_ptr<WMIHelper> m_wmiHelper;

    // Helper methods
    std::string getRegistryString(HKEY _hKey, const std::string& _subKey,
                                  const std::string& _valueName);
    core::StorageDeviceInfo::StorageType determineStorageType(const std::string& _devicePath);
    std::string getVolumeLabel(const std::string& _rootPath);

    // Monitor enumeration callback
    static BOOL CALLBACK monitorEnumProc(HMONITOR _hMonitor, HDC _hdcMonitor, LPRECT _lprcMonitor,
                                         LPARAM _dwData);

   public:
    Win32SystemInfo();
    ~Win32SystemInfo() override = default;

    core::OSInfo getOSInfo() override;
    core::CPUInfo getCPUInfo() override;
    core::MemoryMetrics getMemoryMetrics() override;
    std::vector<core::StorageDeviceInfo> getStorageDevices() override;
    core::LocaleInfo getLocaleInfo() override;
    std::vector<core::MonitorInfo> getMonitors() override;
};

} // namespace win32
} // namespace platform
} // namespace saturn
