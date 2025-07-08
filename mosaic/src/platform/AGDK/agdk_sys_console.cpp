#include "agdk_sys_console.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

// AGDKSystemConsole management methods are no-ops implementations since AGDK uses the
// built-in Android logging system (a.k.a. logcat).

void AGDKSystemConsole::attachParent() {}

void AGDKSystemConsole::detachParent() {}

void AGDKSystemConsole::create() const {}

void AGDKSystemConsole::destroy() const {}

void AGDKSystemConsole::print(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_DEFAULT, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printTrace(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_VERBOSE, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printDebug(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_DEBUG, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printInfo(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_INFO, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printWarn(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_WARN, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printError(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_ERROR, "Mosaic", "%s", _message.c_str());
}

void AGDKSystemConsole::printCritical(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_FATAL, "Mosaic", "%s", _message.c_str());
}

} // namespace agdk
} // namespace platform
} // namespace mosaic
