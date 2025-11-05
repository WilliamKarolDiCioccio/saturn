#include "mosaic/core/logger.hpp"

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <corecrt.h>
#include <ctime>
#include <sstream>
#include <vector>
#include <cassert>
#include <stacktrace>

#include <fmt/format.h>

#include <pieces/core/result.hpp>

#include "mosaic/core/sys_console.hpp"

namespace mosaic
{
namespace core
{

LoggerManager* LoggerManager::s_instance = nullptr;

bool LoggerManager::initialize(const Config& _config) noexcept
{
    assert(s_instance == nullptr && "LoggerManager already exists!");
    s_instance = new LoggerManager();
    return true;
}

void LoggerManager::shutdown() noexcept
{
    assert(s_instance != nullptr && "LoggerManager does not exist!");
    delete s_instance;
    s_instance = nullptr;
}

void LoggerManager::logInternal(LogLevel _level, std::string _formattedMessage) noexcept
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

        localtime_s(&now_tm, &now_c);

        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &now_tm);

        _formattedMessage = fmt::format("[{}] {}", timeBuffer, _formattedMessage);
    }

    // Append stack trace if requested (platform-dependent, stubbed here)
    if (m_config.showStackTrace)
    {
        std::ostringstream oss;
        oss << std::stacktrace::current(); // Trick to get a readable stack trace

        _formattedMessage += fmt::format("\n[Stack Trace]\n{}", oss.str());
    }

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

} // namespace core
} // namespace mosaic
