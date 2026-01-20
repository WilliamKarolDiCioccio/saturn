#include "posix_sys_console.hpp"

#include <iostream>

namespace saturn
{
namespace platform
{
namespace posix
{

void POSIXSystemConsole::attachParent() {}

void POSIXSystemConsole::detachParent() {}

void POSIXSystemConsole::create() const {}

void POSIXSystemConsole::destroy() const {}

void POSIXSystemConsole::print(const std::string& _message) const { std::cout << _message; }

void POSIXSystemConsole::printTrace(const std::string& _message) const { std::cout << _message; }

void POSIXSystemConsole::printDebug(const std::string& _message) const { std::cout << _message; }

void POSIXSystemConsole::printInfo(const std::string& _message) const { std::cout << _message; }

void POSIXSystemConsole::printWarn(const std::string& _message) const { std::cout << _message; }

void POSIXSystemConsole::printError(const std::string& _message) const { std::cerr << _message; }

void POSIXSystemConsole::printCritical(const std::string& _message) const { std::cerr << _message; }

} // namespace posix
} // namespace platform
} // namespace saturn
