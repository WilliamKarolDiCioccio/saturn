#include "mosaic/core/logger_default_sink.hpp"

#include "mosaic/core/sys_console.hpp"

namespace mosaic
{
namespace core
{

pieces::RefResult<core::Sink, std::string> DefaultSink::initialize()
{
    return pieces::OkRef<core::Sink, std::string>(*this);
}

void DefaultSink::shutdown() {}

void DefaultSink::trace(const std::string& _message) const
{
    SystemConsole::printTrace(_message + '\n');
}

void DefaultSink::debug(const std::string& _message) const
{
    SystemConsole::printDebug(_message + '\n');
}

void DefaultSink::info(const std::string& _message) const
{
    SystemConsole::printInfo(_message + '\n');
}

void DefaultSink::warn(const std::string& _message) const
{
    SystemConsole::printWarn(_message + '\n');
}

void DefaultSink::error(const std::string& _message) const
{
    SystemConsole::printError(_message + '\n');
}

void DefaultSink::critical(const std::string& _message) const
{
    SystemConsole::printCritical(_message + '\n');
}

} // namespace core
} // namespace mosaic
