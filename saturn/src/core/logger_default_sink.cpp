#include "saturn/tools/logger_default_sink.hpp"

#include "saturn/core/sys_console.hpp"

namespace saturn
{
namespace tools
{

pieces::RefResult<Sink, std::string> DefaultSink::initialize()
{
    return pieces::OkRef<Sink, std::string>(*this);
}

void DefaultSink::shutdown() {}

void DefaultSink::trace(const std::string& _message) const
{
    core::SystemConsole::printTrace(_message + '\n');
}

void DefaultSink::debug(const std::string& _message) const
{
    core::SystemConsole::printDebug(_message + '\n');
}

void DefaultSink::info(const std::string& _message) const
{
    core::SystemConsole::printInfo(_message + '\n');
}

void DefaultSink::warn(const std::string& _message) const
{
    core::SystemConsole::printWarn(_message + '\n');
}

void DefaultSink::error(const std::string& _message) const
{
    core::SystemConsole::printError(_message + '\n');
}

void DefaultSink::critical(const std::string& _message) const
{
    core::SystemConsole::printCritical(_message + '\n');
}

} // namespace tools
} // namespace saturn
