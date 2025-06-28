#include "emscripten_sys_console.hpp"

namespace mosaic
{
namespace platform
{
namespace emscripten
{

void EmscriptenSystemConsole::redirect() const
{
    // No redirection needed for Emscripten
}

void EmscriptenSystemConsole::restore() const
{
    // No restoration needed for Emscripten
}

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
} // namespace mosaic
