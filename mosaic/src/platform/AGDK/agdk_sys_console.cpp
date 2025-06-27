#include "agdk_sys_console.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

void AGDKSystemConsole::redirect() const
{
    // No redirection needed for Android
}

void AGDKSystemConsole::restore() const
{
    // No restoration needed for Android
}

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
