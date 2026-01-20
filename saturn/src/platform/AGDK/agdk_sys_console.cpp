#include "agdk_sys_console.hpp"

namespace saturn
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
    __android_log_print(ANDROID_LOG_DEFAULT, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printTrace(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_VERBOSE, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printDebug(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_DEBUG, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printInfo(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_INFO, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printWarn(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_WARN, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printError(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_ERROR, "Saturn", "%s", _message.c_str());
}

void AGDKSystemConsole::printCritical(const std::string& _message) const
{
    __android_log_print(ANDROID_LOG_FATAL, "Saturn", "%s", _message.c_str());
}

} // namespace agdk
} // namespace platform
} // namespace saturn
