#include "emscripten_sys_console.hpp"

namespace saturn
{
namespace platform
{
namespace emscripten
{

// EmscriptenSystemConsole management methods are no-ops implementations since Emscripten uses the
// built-in browser's console.

void EmscriptenSystemConsole::attachParent() {}

void EmscriptenSystemConsole::detachParent() {}

void EmscriptenSystemConsole::create() const {}

void EmscriptenSystemConsole::destroy() const {}

void EmscriptenSystemConsole::print(const std::string& _message) const
{
    emscripten_log(NULL, "%s", _message.c_str());
}

void EmscriptenSystemConsole::printTrace(const std::string& _message) const
{
    emscripten_log(EM_LOG_CONSOLE, "TRACE: %s", _message.c_str());
}

void EmscriptenSystemConsole::printDebug(const std::string& _message) const
{
    emscripten_log(EM_LOG_DEBUG, "DEBUG: %s", _message.c_str());
}

void EmscriptenSystemConsole::printInfo(const std::string& _message) const
{
    emscripten_log(EM_LOG_INFO, "INFO: %s", _message.c_str());
}

void EmscriptenSystemConsole::printWarn(const std::string& _message) const
{
    emscripten_log(EM_LOG_WARN, "WARN: %s", _message.c_str());
}

void EmscriptenSystemConsole::printError(const std::string& _message) const
{
    emscripten_log(EM_LOG_ERROR, "ERROR: %s", _message.c_str());
}

void EmscriptenSystemConsole::printCritical(const std::string& _message) const
{
    emscripten_log(EM_LOG_ERROR, "CRITICAL: %s", _message.c_str());
}

} // namespace emscripten
} // namespace platform
} // namespace saturn
