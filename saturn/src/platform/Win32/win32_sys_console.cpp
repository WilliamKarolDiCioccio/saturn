#include "win32_sys_console.hpp"

#include <iostream>

#include <windows.h>

#include <colorconsole.hpp>

namespace saturn
{
namespace platform
{
namespace win32
{

constexpr auto k_consoleBufferMinLength = 256;

/**
 * @brief Adjusts the console buffer size to ensure it is at least the specified minimum length.
 *
 * This function ensures the console has enough vertical space to properly display output
 * when attaching late from a GUI subsystem application (e.g. via AttachConsole).
 * Without this, brief outputs (like error messages) may appear overwritten
 * or jumbled with the shell prompt. This function increases the screen buffer
 * height if it's smaller than a given minimum.
 *
 * @param minLength
 */
void AdjustConsoleBuffer(int16_t minLength)
{
    CONSOLE_SCREEN_BUFFER_INFO conInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);
    if (conInfo.dwSize.Y < minLength) conInfo.dwSize.Y = minLength;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);
}

void Win32SystemConsole::attachParent()
{
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AdjustConsoleBuffer(k_consoleBufferMinLength);

        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::ios::sync_with_stdio(true);

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }
    else
    {
        create();
    }
}

void Win32SystemConsole::detachParent()
{
    fflush(stdout);
    fflush(stderr);

    FILE* fp;
    freopen_s(&fp, "NUL", "w", stdout);
    freopen_s(&fp, "NUL", "w", stderr);
    freopen_s(&fp, "NUL", "r", stdin);

    FreeConsole();
}

void Win32SystemConsole::create() const
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

void Win32SystemConsole::destroy() const { FreeConsole(); }

void Win32SystemConsole::print(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::white(_message);
}

void Win32SystemConsole::printTrace(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::grey(_message);
}

void Win32SystemConsole::printDebug(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::blue(_message);
}

void Win32SystemConsole::printInfo(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::green(_message);
}

void Win32SystemConsole::printWarn(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cout << dye::yellow(_message);
}

void Win32SystemConsole::printError(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cerr << dye::red(_message);
}

void Win32SystemConsole::printCritical(const std::string& _message) const
{
#if defined(SATURN_DEBUG_BUILD)
    OutputDebugStringA(_message.c_str());
#endif
    std::cerr << dye::purple(_message);
}

} // namespace win32
} // namespace platform
} // namespace saturn
