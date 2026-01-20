#include "saturn/tools/logger.hpp"

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <sstream>
#include <vector>
#include <cassert>

#ifndef SATURN_ARCH_ARM64
#include <stacktrace>
#endif

#ifdef SATURN_COMPILER_MSVC
#include <corecrt.h>
#else
#include <time.h>
#endif

#include <fmt/format.h>

#include <pieces/core/result.hpp>

#include "saturn/core/sys_console.hpp"

namespace saturn
{
namespace tools
{

Logger* Logger::s_instance = nullptr;

bool Logger::initialize(const Config& _config) noexcept
{
    assert(s_instance == nullptr && "Logger already exists!");
    s_instance = new Logger();
    return true;
}

void Logger::shutdown() noexcept
{
    assert(s_instance != nullptr && "Logger does not exist!");
    delete s_instance;
    s_instance = nullptr;
}

void Logger::logInternal(LogLevel _level, std::string _formattedMessage) noexcept
{
    // Prepend log level if requested
    if (m_config.showLevel)
    {
        _formattedMessage =
            fmt::format("[{}] {}", c_levelNames[static_cast<int>(_level)], _formattedMessage);
    }

    auto tid = std::this_thread::get_id(); // Moved outside the scope for reuse

    // Prepend thread ID if requested
    if (m_config.showTid)
    {
        std::ostringstream oss;
        oss << tid; // Trick to get a readable thread ID

        _formattedMessage = fmt::format("[{}] {}", oss.str(), _formattedMessage);
    }

    // Prepend timestamp if requested
    if (m_config.showTimestamp)
    {
        auto now = std::chrono::system_clock::now();
        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;

#ifdef SATURN_COMPILER_MSVC
        localtime_s(&now_tm, &now_c);
#else
        localtime_r(&now_c, &now_tm);
#endif

        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &now_tm);

        _formattedMessage = fmt::format("[{}] {}", timeBuffer, _formattedMessage);
    }

#ifndef SATURN_ARCH_ARM64
    // Append stack trace if requested (platform-dependent, stubbed here)
    if (m_config.showStackTrace)
    {
        std::ostringstream oss;
        oss << std::stacktrace::current(); // Trick to get a readable stack trace

        _formattedMessage += fmt::format("\n[Stack Trace]\n{}", oss.str());
    }
#endif

    for (const auto& [name, sink] : m_sinks)
    {
        switch (_level)
        {
            case LogLevel::trace:
                sink->trace(_formattedMessage);
                break;
            case LogLevel::debug:
                sink->debug(_formattedMessage);
                break;
            case LogLevel::info:
                sink->info(_formattedMessage);
                break;
            case LogLevel::warn:
                sink->warn(_formattedMessage);
                break;
            case LogLevel::error:
                sink->error(_formattedMessage);
                break;
            case LogLevel::critical:
                sink->critical(_formattedMessage);
                break;
            default:
                break;
        }
    }

    if (m_history.find(tid) == m_history.end())
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_history[tid] = std::vector<std::string>();
    }

    if (m_history.at(tid).size() >= m_config.historySize) clearHistory();

    m_history.at(tid).emplace_back(_formattedMessage);
}

} // namespace tools
} // namespace saturn
