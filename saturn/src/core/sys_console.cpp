#include "saturn/core/sys_console.hpp"

#include <memory>
#include <string>

#include "saturn/defines.hpp"

#if defined(SATURN_PLATFORM_WINDOWS)
#include "platform/Win32/win32_sys_console.hpp"
#elif defined(SATURN_PLATFORM_LINUX)
#include "platform/POSIX/posix_sys_console.hpp"
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
#include "platform/Emscripten/emscripten_sys_console.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_sys_console.hpp"
#endif

namespace saturn
{
namespace core
{

#if defined(SATURN_PLATFORM_WINDOWS)
std::unique_ptr<SystemConsole::SystemConsoleImpl> SystemConsole::impl =
    std::make_unique<platform::win32::Win32SystemConsole>();
#elif defined(SATURN_PLATFORM_LINUX)
std::unique_ptr<SystemConsole::SystemConsoleImpl> SystemConsole::impl =
    std::make_unique<platform::posix::POSIXSystemConsole>();
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
std::unique_ptr<SystemConsole::SystemConsoleImpl> SystemConsole::impl =
    std::make_unique<platform::emscripten::EmscriptenSystemConsole>();
#elif defined(SATURN_PLATFORM_ANDROID)
std::unique_ptr<SystemConsole::SystemConsoleImpl> SystemConsole::impl =
    std::make_unique<platform::agdk::AGDKSystemConsole>();
#endif

void SystemConsole::attachParent() { impl->attachParent(); }

void SystemConsole::detachParent() { impl->detachParent(); }

void SystemConsole::create() { impl->create(); }

void SystemConsole::destroy() { impl->destroy(); }

void SystemConsole::print(const std::string& _message) { impl->print(_message); }

void SystemConsole::printTrace(const std::string& _message) { impl->printTrace(_message); }

void SystemConsole::printDebug(const std::string& _message) { impl->printDebug(_message); }

void SystemConsole::printInfo(const std::string& _message) { impl->printInfo(_message); }

void SystemConsole::printWarn(const std::string& _message) { impl->printWarn(_message); }

void SystemConsole::printError(const std::string& _message) { impl->printError(_message); }

void SystemConsole::printCritical(const std::string& _message) { impl->printCritical(_message); }

} // namespace core
} // namespace saturn
