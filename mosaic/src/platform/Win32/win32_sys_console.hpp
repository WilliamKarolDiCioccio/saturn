#include "mosaic/core/sys_console.hpp"

#include <windows.h>
#include <iostream>

namespace mosaic
{
namespace platform
{
namespace win32
{

class Win32SystemConsole : public core::SystemConsole::SystemConsoleImpl
{
   public:
    Win32SystemConsole() = default;
    ~Win32SystemConsole() override = default;

   public:
    void redirect() const override;
    void restore() const override;

    void print(const std::string& _message) const override;
    void printTrace(const std::string& _message) const override;
    void printDebug(const std::string& _message) const override;
    void printInfo(const std::string& _message) const override;
    void printWarn(const std::string& _message) const override;
    void printError(const std::string& _message) const override;
    void printCritical(const std::string& _message) const override;
};

} // namespace win32
} // namespace platform
} // namespace mosaic
