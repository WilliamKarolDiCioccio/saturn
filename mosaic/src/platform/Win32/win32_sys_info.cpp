#include "win32_sys_info.hpp"

#include <windows.h>
#include <sysinfoapi.h>
#include <ShellScalingApi.h>
#include <versionhelpers.h>
#include <winreg.h>
#include <psapi.h>
#include <locale.h>
#include <shlobj.h>
#include <winbase.h>
#include <powerbase.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <immintrin.h>
#include <isa_availability.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "shcore.lib")

namespace saturn
{
namespace platform
{
namespace win32
{

Win32SystemInfo::WMIHelper::WMIHelper() : m_pLoc(nullptr), m_pSvc(nullptr), m_initialized(false)
{
    initialize();
}

Win32SystemInfo::WMIHelper::~WMIHelper() { shutdown(); }

bool Win32SystemInfo::WMIHelper::initialize()
{
    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                    (LPVOID*)&m_pLoc);
    if (FAILED(hres)) return false;

    hres = m_pLoc->ConnectServer(bstr_t("ROOT\\CIMV2"), NULL, NULL, 0,
                                 WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &m_pSvc);

    if (FAILED(hres))
    {
        m_pLoc->Release();

        return false;
    }

    hres = CoSetProxyBlanket(m_pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres))
    {
        m_pSvc->Release();
        m_pLoc->Release();

        return false;
    }

    return m_initialized = true;
}

void Win32SystemInfo::WMIHelper::shutdown()
{
    m_pSvc->Release();
    m_pLoc->Release();
}

uint32_t Win32SystemInfo::WMIHelper::queryWMIUInt32(const std::wstring& query,
                                                    const std::wstring& property)
{
    if (!m_initialized) return 0;

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = m_pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                     &pEnumerator);
    if (FAILED(hres)) return 0;

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    uint32_t result = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp{};
        hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4) result = vtProp.lVal;

        VariantClear(&vtProp);
        pclsObj->Release();

        break;
    }

    pEnumerator->Release();

    return result;
}

uint64_t Win32SystemInfo::WMIHelper::queryWMIUInt64(const std::wstring& query,
                                                    const std::wstring& property)
{
    if (!m_initialized) return 0;

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = m_pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                     &pEnumerator);
    if (FAILED(hres)) return 0;

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    uint64_t result = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp{};
        hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr))
        {
            if (vtProp.vt == VT_I8)
            {
                result = vtProp.llVal;
            }
            else if (vtProp.vt == VT_I4)
            {
                result = vtProp.lVal;
            }
        }

        VariantClear(&vtProp);
        pclsObj->Release();

        break;
    }

    pEnumerator->Release();

    return result;
}

std::string Win32SystemInfo::WMIHelper::queryWMIString(const std::wstring& query,
                                                       const std::wstring& property)
{
    if (!m_initialized) return "";

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = m_pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                     &pEnumerator);
    if (FAILED(hres)) return "";

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    std::string result;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp{};
        hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) result = WStringToString(vtProp.bstrVal);

        VariantClear(&vtProp);
        pclsObj->Release();

        break;
    }

    pEnumerator->Release();

    return result;
}

std::vector<std::string> Win32SystemInfo::WMIHelper::queryWMIStringArray(
    const std::wstring& query, const std::wstring& property)
{
    std::vector<std::string> results;
    if (!m_initialized) return results;

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = m_pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                     &pEnumerator);
    if (FAILED(hres)) return results;

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp{};
        hr = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            results.push_back(WStringToString(vtProp.bstrVal));
        }

        VariantClear(&vtProp);
        pclsObj->Release();
    }

    pEnumerator->Release();

    return results;
}

Win32SystemInfo::Win32SystemInfo() : m_wmiHelper(nullptr) {}

core::OSInfo Win32SystemInfo::getOSInfo()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    core::OSInfo osInfo{};

    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    std::string productName = getRegistryString(
        HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName");
    std::string currentVersion = getRegistryString(
        HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentVersion");
    std::string buildNumber = getRegistryString(
        HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild");

    osInfo.name = productName.empty() ? "Windows" : productName;
    osInfo.version = currentVersion;
    osInfo.kernelVersion = currentVersion,
    osInfo.kernelVersion = currentVersion + "." + buildNumber;
    osInfo.build = buildNumber;

    return osInfo;
}

core::CPUInfo Win32SystemInfo::getCPUInfo()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    core::CPUInfo info{};

    info.model = m_wmiHelper->queryWMIString(L"SELECT Name FROM Win32_Processor", L"Name");

    info.vendor =
        m_wmiHelper->queryWMIString(L"SELECT Manufacturer FROM Win32_Processor", L"Manufacturer");

    uint32_t architectureCode =
        m_wmiHelper->queryWMIUInt32(L"SELECT Architecture FROM Win32_Processor", L"Architecture");

    switch (architectureCode)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            info.architecture = "x86_64";
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            info.architecture = "arm64";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            info.architecture = "x86";
            break;
        default:
            info.architecture = "unknown";
            break;
    }

    info.physicalCores =
        m_wmiHelper->queryWMIUInt32(L"SELECT NumberOfCores FROM Win32_Processor", L"NumberOfCores");

    info.logicalCores = m_wmiHelper->queryWMIUInt32(
        L"SELECT NumberOfLogicalProcessors FROM Win32_Processor", L"NumberOfLogicalProcessors");

    uint32_t maxClockSpeed =
        m_wmiHelper->queryWMIUInt32(L"SELECT MaxClockSpeed FROM Win32_Processor", L"MaxClockSpeed");
    info.clockSpeedGHz = maxClockSpeed / 1000.0f;

    info.cacheSizeKB =
        m_wmiHelper->queryWMIUInt32(L"SELECT L3CacheSize FROM Win32_Processor", L"L3CacheSize");

    auto checkISA = [](unsigned int _feature, unsigned int _version = 0) -> bool
    {
        return (__check_arch_support((_feature), (_version)) ||
                __check_isa_support((_feature), (_version)));
    };

    // The __check_isa_support() function is not meant to give a feature by feature support
    // checklist, but rather ensure the availability of specific ISA features with major impact on
    // MSVC's code generation and optimization. In this code we assume backward compatibility, so if
    // a newer ISA is supported, we assume older ones are too.

    // We're not yet supporting Windows on ARM, so we default to false for NEON support
    info.isaSupport.neon = false;

    if (checkISA(__IA_SUPPORT_VECTOR128))
    {
        info.isaSupport.sse2 = true;
    }

    if (checkISA(__IA_SUPPORT_SSE42))
    {
        info.isaSupport.sse42 = true;
        info.isaSupport.sse41 = true;
        info.isaSupport.ssse3 = true;
    }

    if (checkISA(__IA_SUPPORT_VECTOR256))
    {
        info.isaSupport.avx2 = true;
        info.isaSupport.avx = true;
    }

    info.isaSupport.avx512 = checkISA(__IA_SUPPORT_VECTOR512);
    info.isaSupport.avx10 = checkISA(__IA_SUPPORT_AVX10);
    info.isaSupport.avx10_2 = checkISA(__IA_SUPPORT_AVX10_2);
    info.isaSupport.apx = checkISA(__IA_SUPPORT_APX);
    info.isaSupport.fp16 = checkISA(__IA_SUPPORT_FP16);

    return info;
}

core::MemoryMetrics Win32SystemInfo::getMemoryMetrics()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    core::MemoryMetrics metrics{};

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);

    if (GlobalMemoryStatusEx(&memStatus))
    {
        metrics.totalMemoryKB = BYTES_TO_KB(memStatus.ullTotalPhys);
        metrics.freeMemoryKB = BYTES_TO_KB(memStatus.ullAvailPhys);
        metrics.usedMemoryKB = metrics.totalMemoryKB - metrics.freeMemoryKB;
    }

    return metrics;
}

std::vector<core::StorageDeviceInfo> Win32SystemInfo::getStorageDevices()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    std::vector<core::StorageDeviceInfo> devicesInfo;

    DWORD drivesMask = GetLogicalDrives();

    for (int i = 0; i < 26; i++)
    {
        if (drivesMask & (1 << i))
        {
            char driveLetter = 'A' + i;
            std::string rootPath = std::string(1, driveLetter) + ":\\";

            UINT driveType = GetDriveTypeA(rootPath.c_str());
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE ||
                driveType == DRIVE_REMOTE)
            {
                core::StorageDeviceInfo device;

                device.mountPoint = rootPath;
                device.name = std::string(1, driveLetter) + ":";

                switch (driveType)
                {
                    case DRIVE_FIXED:
                        device.type = determineStorageType(rootPath);
                        break;
                    case DRIVE_REMOVABLE:
                        device.type = core::StorageDeviceInfo::StorageType::USB;
                        break;
                    case DRIVE_REMOTE:
                        device.type = core::StorageDeviceInfo::StorageType::Network;
                        break;
                    default:
                        device.type = core::StorageDeviceInfo::StorageType::HDD;
                        break;
                }

                ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                if (GetDiskFreeSpaceExA(rootPath.c_str(), &freeBytesAvailable, &totalNumberOfBytes,
                                        &totalNumberOfFreeBytes))
                {
                    device.metrics.totalStorageKB = BYTES_TO_KB(totalNumberOfBytes.QuadPart);
                    device.metrics.freeStorageKB = BYTES_TO_KB(totalNumberOfFreeBytes.QuadPart);
                    device.metrics.usedStorageKB =
                        device.metrics.totalStorageKB - device.metrics.freeStorageKB;
                }

                devicesInfo.push_back(device);
            }
        }
    }

    return devicesInfo;
}

core::LocaleInfo Win32SystemInfo::getLocaleInfo()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    core::LocaleInfo info;

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetSystemDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
    {
        info.locale = WStringToString(localeName);
    }

    size_t dashPos = info.locale.find('-');
    if (dashPos != std::string::npos)
    {
        info.languageCode = info.locale.substr(0, dashPos);
        info.countryCode = info.locale.substr(dashPos + 1);
    }

    info.encoding = "UTF-8";

    TIME_ZONE_INFORMATION tzInfo;
    DWORD tzResult = GetTimeZoneInformation(&tzInfo);
    if (tzResult != TIME_ZONE_ID_INVALID)
    {
        info.timezone = WStringToString(tzInfo.StandardName);
        // Windows bias is negative of standard offset
        info.timezoneOffsetMinutes = -tzInfo.Bias;
    }

    wchar_t currencySymbol[10];
    if (GetLocaleInfoEx(localeName, LOCALE_SCURRENCY, currencySymbol, 10))
    {
        info.currencySymbol = WStringToString(currencySymbol);
    }

    wchar_t currencyCode[10];
    if (GetLocaleInfoEx(localeName, LOCALE_SINTLSYMBOL, currencyCode, 10))
    {
        info.currencyCode = WStringToString(currencyCode);
    }

    wchar_t shortDateFormat[80];
    if (GetLocaleInfoEx(localeName, LOCALE_SSHORTDATE, shortDateFormat, 80))
    {
        info.shortDateFormat = WStringToString(shortDateFormat);
    }

    wchar_t longDateFormat[80];
    if (GetLocaleInfoEx(localeName, LOCALE_SLONGDATE, longDateFormat, 80))
    {
        info.longDateFormat = WStringToString(longDateFormat);
    }

    wchar_t timeFormat[80];
    if (GetLocaleInfoEx(localeName, LOCALE_STIMEFORMAT, timeFormat, 80))
    {
        info.timeFormat = WStringToString(timeFormat);
    }

    wchar_t decimalSep[10];
    if (GetLocaleInfoEx(localeName, LOCALE_SDECIMAL, decimalSep, 10))
    {
        std::string decStr = WStringToString(decimalSep);
        if (!decStr.empty()) info.decimalSeparator = decStr[0];
    }

    wchar_t thousandsSep[10];
    if (GetLocaleInfoEx(localeName, LOCALE_STHOUSAND, thousandsSep, 10))
    {
        std::string thouStr = WStringToString(thousandsSep);
        if (!thouStr.empty()) info.thousandsSeparator = thouStr[0];
    }

    wchar_t firstDay[10];
    if (GetLocaleInfoEx(localeName, LOCALE_IFIRSTDAYOFWEEK, firstDay, 10))
    {
        int dayNum = _wtoi(firstDay);

        switch (dayNum)
        {
            case 0:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Monday;
                break;
            case 1:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Tuesday;
                break;
            case 2:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Wednesday;
                break;
            case 3:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Thursday;
                break;
            case 4:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Friday;
                break;
            case 5:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Saturday;
                break;
            case 6:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Sunday;
                break;
            default:
                info.firstDayOfWeek = core::LocaleInfo::Weekday::Sunday;
                break;
        }
    }

    return info;
}

std::vector<core::MonitorInfo> Win32SystemInfo::getMonitors()
{
    if (!m_wmiHelper) m_wmiHelper = std::make_unique<WMIHelper>();

    std::vector<core::MonitorInfo> monitorsInfo;

    EnumDisplayMonitors(NULL, NULL, monitorEnumProc, reinterpret_cast<LPARAM>(&monitorsInfo));

    return monitorsInfo;
}

std::string Win32SystemInfo::getRegistryString(HKEY hKey, const std::string& subKey,
                                               const std::string& valueName)
{
    HKEY key;
    if (RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return "";

    DWORD dataSize = 0;
    if (RegQueryValueExA(key, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize) !=
        ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return "";
    }

    std::string result(dataSize, '\0');
    if (RegQueryValueExA(key, valueName.c_str(), nullptr, nullptr,
                         reinterpret_cast<LPBYTE>(&result[0]), &dataSize) != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return "";
    }

    RegCloseKey(key);

    size_t nullPos = result.find('\0');
    if (nullPos != std::string::npos) result.resize(nullPos);

    return result;
}

core::StorageDeviceInfo::StorageType Win32SystemInfo::determineStorageType(
    const std::string& devicePath)
{
    std::wstring query = L"SELECT MediaType FROM Win32_PhysicalMedia WHERE Tag LIKE '%" +
                         StringToWString(devicePath.substr(0, 1)) + L"%'";

    std::string mediaType = m_wmiHelper->queryWMIString(query, L"MediaType");

    if (mediaType.find("SSD") != std::string::npos ||
        mediaType.find("Solid State") != std::string::npos)
    {
        return core::StorageDeviceInfo::StorageType::SSD;
    }
    else
    {
        return core::StorageDeviceInfo::StorageType::HDD;
    }
}

std::string Win32SystemInfo::getVolumeLabel(const std::string& rootPath)
{
    char volumeName[MAX_PATH + 1];
    DWORD volumeSerialNumber, maxComponentLength, fileSystemFlags;
    char fileSystemName[MAX_PATH + 1];

    if (GetVolumeInformationA(rootPath.c_str(), volumeName, sizeof(volumeName), &volumeSerialNumber,
                              &maxComponentLength, &fileSystemFlags, fileSystemName,
                              sizeof(fileSystemName)))
    {
        return std::string(volumeName);
    }

    return "";
}

BOOL CALLBACK Win32SystemInfo::monitorEnumProc(HMONITOR hMonitor, [[maybe_unused]] HDC hdcMonitor,
                                               [[maybe_unused]] LPRECT lprcMonitor, LPARAM dwData)
{
    std::vector<core::MonitorInfo>* monitors =
        reinterpret_cast<std::vector<core::MonitorInfo>*>(dwData);

    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);

    if (GetMonitorInfoA(hMonitor, &monitorInfo))
    {
        // This section will be improved in the future using the EDID API

        core::MonitorInfo info;

        info.name = monitorInfo.szDevice;
        info.model = monitorInfo.szDevice;

        info.resolution.x = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        info.resolution.y = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

        info.position.x = monitorInfo.rcMonitor.left;
        info.position.y = monitorInfo.rcMonitor.top;

        info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

        float aspectRatio =
            static_cast<float>(info.resolution.x) / static_cast<float>(info.resolution.y);
        info.aspectRatio.x = aspectRatio;
        info.aspectRatio.y = 1.0f;

        DEVMODE devMode;
        devMode.dmSize = sizeof(DEVMODE);
        if (EnumDisplaySettingsA(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode))
        {
            info.refreshRate = devMode.dmDisplayFrequency;
        }

        info.isConnected = true;
        info.isEnabled = true;
        info.orientation = core::MonitorInfo::Orientation::Landscape;
        info.colorDepth = core::MonitorInfo::ColorDepth::Bit8;
        info.colorSpace = core::MonitorInfo::ColorSpace::sRGB;

        UINT dpiX, dpiY;
        GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        info.dpi.x = static_cast<float>(dpiX);
        info.dpi.y = static_cast<float>(dpiY);

        monitors->push_back(info);
    }

    return TRUE;
}

} // namespace win32
} // namespace platform
} // namespace saturn
