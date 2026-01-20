#include "saturn/core/sys_info.hpp"

#include <memory>
#include <vector>

#include "saturn/defines.hpp"

#if defined(SATURN_PLATFORM_WINDOWS)
#include "platform/Win32/win32_sys_info.hpp"
#elif defined(SATURN_PLATFORM_LINUX)
#include "platform/POSIX/posix_sys_info.hpp"
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
#include "platform/Emscripten/emscripten_sys_info.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_sys_info.hpp"
#endif

namespace saturn
{
namespace core
{

#if defined(SATURN_PLATFORM_WINDOWS)
std::unique_ptr<SystemInfo::SystemInfoImpl> SystemInfo::impl =
    std::make_unique<platform::win32::Win32SystemInfo>();
#elif defined(SATURN_PLATFORM_LINUX)
std::unique_ptr<SystemInfo::SystemInfoImpl> SystemInfo::impl =
    std::make_unique<platform::posix::POSIXSystemInfo>();
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
std::unique_ptr<SystemInfo::SystemInfoImpl> SystemInfo::impl =
    std::make_unique<platform::emscripten::EmscriptenSystemInfo>();
#elif defined(SATURN_PLATFORM_ANDROID)
std::unique_ptr<SystemInfo::SystemInfoImpl> SystemInfo::impl =
    std::make_unique<platform::agdk::AGDKSystemInfo>();
#endif

OSInfo SystemInfo::getOSInfo() { return impl->getOSInfo(); }

CPUInfo SystemInfo::getCPUInfo() { return impl->getCPUInfo(); }

MemoryMetrics SystemInfo::getMemoryMetrics() { return impl->getMemoryMetrics(); }

std::vector<StorageDeviceInfo> SystemInfo::getStorageDevices() { return impl->getStorageDevices(); }

LocaleInfo SystemInfo::getLocaleInfo() { return impl->getLocaleInfo(); }

std::vector<MonitorInfo> SystemInfo::getMonitors() { return impl->getMonitors(); }

} // namespace core
} // namespace saturn
