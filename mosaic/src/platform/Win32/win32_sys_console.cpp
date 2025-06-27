#include "win32_sys_console.hpp"

#include <colorconsole.hpp>

namespace mosaic
{
namespace platform
{
namespace win32
{

void Win32SystemConsole::redirect() const
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::ios::sync_with_stdio(true);
}

void Win32SystemConsole::restore() const { FreeConsole(); }

void Win32SystemConsole::print(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::white(_message);
}

void Win32SystemConsole::printTrace(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::grey(_message);
}

void Win32SystemConsole::printDebug(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::blue(_message);
}

void Win32SystemConsole::printInfo(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::green(_message);
}

void Win32SystemConsole::printWarn(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::yellow(_message);
}

void Win32SystemConsole::printError(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cerr << dye::red(_message);
}

void Win32SystemConsole::printCritical(const std::string& _message) const
{
#if defined(MOSAIC_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cerr << dye::purple(_message);
}

} // namespace win32
} // namespace platform
} // namespace mosaic
