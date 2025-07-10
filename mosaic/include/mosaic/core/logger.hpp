#pragma once

#include "mosaic/defines.hpp"

#include <memory>
#include <string>
#include <array>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <stacktrace>

#include <fmt/format.h>

#include <pieces/result.hpp>

#include "mosaic/core/sys_console.hpp"

namespace mosaic
{
namespace core
{

constexpr uint16_t k_logHistorySize = 1024;
constexpr const char* k_logsBasePath = "logs/";

class Sink;

template <typename SinkType>
concept IsSink = std::derived_from<SinkType, Sink> && !std::is_abstract_v<SinkType>;

/**
 * @brief Base class for interfaces providing logging functionality.
 */
class Sink
{
   public:
    virtual ~Sink() = default;

    virtual pieces::RefResult<Sink, std::string> initialize() = 0;
    virtual void shutdown() = 0;

    virtual void trace(const std::string& _message) const = 0;
    virtual void debug(const std::string& _message) const = 0;
    virtual void info(const std::string& _message) const = 0;
    virtual void warn(const std::string& _message) const = 0;
    virtual void error(const std::string& _message) const = 0;
    virtual void critical(const std::string& _message) const = 0;
};

/**
 * @brief Default logging sink that outputs logs to the console.
 */
class MOSAIC_API DefaultSink final : public Sink
{
   public:
    ~DefaultSink() override = default;

    pieces::RefResult<Sink, std::string> initialize() override;
    void shutdown() override;

    void trace(const std::string& _message) const override;
    void debug(const std::string& _message) const override;
    void info(const std::string& _message) const override;
    void warn(const std::string& _message) const override;
    void error(const std::string& _message) const override;
    void critical(const std::string& _message) const override;
};

enum class LogLevel
{
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5
};

/**
 * @brief Lookup table for log level names.
 */
constexpr std::array<const char*, 6> c_levelNames = {
    "Trace", "Debug", "Info", "Warn", "Error", "Critical",
};

/**
 * @brief Configuration for the LoggerManager.
 *
 * This struct allows customization of logging behavior, such as enabling/disabling log levels,
 * showing thread IDs, timestamps, and stack traces, as well as setting the history size
 * and base logs path.
 */
struct LoggerConfig
{
    std::atomic_bool showLevel;
    std::atomic_bool showTid;
    std::atomic_bool showTimestamp;
    std::atomic_bool showStackTrace;

    std::atomic_uint16_t historySize;

    std::array<std::atomic_bool, 6> levelEnabled;

    LoggerConfig(bool _showLevel = true, bool _showTid = false, bool _showTimestamp = true,
                 bool _showStackTrace = false, uint16_t _historySize = k_logHistorySize)
        : showLevel(_showLevel),
          showTid(_showTid),
          showTimestamp(_showTimestamp),
          showStackTrace(_showStackTrace),
          historySize(_historySize)
    {
        for (auto& e : levelEnabled) e.store(true);
    }

    LoggerConfig(const LoggerConfig& other)
        : showLevel(other.showLevel.load()),
          showTid(other.showTid.load()),
          showTimestamp(other.showTimestamp.load()),
          showStackTrace(other.showStackTrace.load()),
          historySize(other.historySize.load())
    {
        for (size_t i = 0; i < levelEnabled.size(); ++i)
        {
            levelEnabled[i].store(other.levelEnabled[i].load());
        }
    }
};

class MOSAIC_API LoggerManager final
{
   private:
    static LoggerManager* s_instance;

    std::unordered_map<std::string, std::shared_ptr<Sink>> m_sinks;
    std::mutex m_sinksMutex;
    std::unordered_map<std::thread::id, std::vector<std::string>> m_history;
    std::mutex m_historyMutex;

    LoggerConfig m_config;

   private:
    LoggerManager(const LoggerConfig& _config);

   public:
    static bool initialize(const LoggerConfig& _config = LoggerConfig()) noexcept;
    static void shutdown() noexcept;

    // Configurable options

    void setShowLevel(bool _show) noexcept { m_config.showLevel = _show; }
    [[nodiscard]] bool isShowLevel() const noexcept { return m_config.showLevel; }

    void setShowTid(bool _show) noexcept { m_config.showTid = _show; }
    [[nodiscard]] bool isShowTid() const noexcept { return m_config.showTid; }

    void setShowTimestamp(bool _show) noexcept { m_config.showTimestamp = _show; }
    [[nodiscard]] bool isShowTimestamp() const noexcept { return m_config.showTimestamp; }

    void setShowStackTrace(bool _show) noexcept { m_config.showStackTrace = _show; }
    [[nodiscard]] bool isShowStackTrace() const noexcept { return m_config.showStackTrace; }

    void setHistorySize(uint16_t _size) noexcept
    {
        m_config.historySize = _size;
        m_history.reserve(_size);
    }

    [[nodiscard]] uint16_t getHistorySize() const noexcept { return m_config.historySize; }

    void enableLevel(LogLevel _level, bool _enabled) noexcept
    {
        m_config.levelEnabled[static_cast<int>(_level)] = _enabled;
    }

    [[nodiscard]] bool isLevelEnabled(LogLevel _level) const noexcept
    {
        return m_config.levelEnabled[static_cast<int>(_level)];
    }

    // Sink management

    template <typename SinkType>
    bool addSink(const std::string& _name, SinkType&& _sink) noexcept
        requires IsSink<SinkType>
    {
        if (m_sinks.find(_name) != m_sinks.end()) return false;

        std::lock_guard<std::mutex> lock(m_sinksMutex);

        auto sink = std::make_shared<SinkType>(std::forward<SinkType>(_sink));

        auto result = sink->initialize();

        if (result.isErr())
        {
            SystemConsole::printError(
                fmt::format("Failed to initialize sink '{}': {}", _name, result.error()));
            return false;
        }

        m_sinks.insert({_name, sink});

        return true;
    }

    void removeSink(const std::string& _name) noexcept
    {
        if (m_sinks.find(_name) == m_sinks.end()) return;

        std::lock_guard<std::mutex> lock(m_sinksMutex);

        m_sinks.at(_name)->shutdown();

        m_sinks.erase(_name);
    }

    void clearSinks() noexcept
    {
        std::lock_guard<std::mutex> lock(m_sinksMutex);

        m_sinks.clear();
    }

    // History management

    [[nodiscard]] const std::vector<std::string>& getHistory() const noexcept
    {
        auto tid = std::this_thread::get_id();

        if (m_history.find(tid) == m_history.end()) return {};

        return m_history.at(tid);
    }

    void clearHistory() noexcept { m_history[std::this_thread::get_id()].clear(); }

    // Logging methods

    template <typename... Args>
    void log(LogLevel _level, const std::string& _message, Args&&... _args) noexcept
    {
        try
        {
            if (!s_instance) throw std::runtime_error("LoggerManager is not initialized");

            if (!m_config.levelEnabled[static_cast<int>(_level)]) return;

            // Format message with arguments
            std::string formattedMessage = fmt::vformat(_message, fmt::make_format_args(_args...));

            // Prepend log level if requested
            if (m_config.showLevel)
            {
                formattedMessage = fmt::format("[{}] {}", c_levelNames[static_cast<int>(_level)],
                                               formattedMessage);
            }

            auto tid = std::this_thread::get_id(); // Moved outside the scope for reuse

            // Prepend thread ID if requested
            if (m_config.showTid)
            {
                std::ostringstream oss;
                oss << tid; // Trick to get a readable thread ID

                formattedMessage = fmt::format("[{}] {}", oss.str(), formattedMessage);
            }

            // Prepend timestamp if requested
            if (m_config.showTimestamp)
            {
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                std::tm* now_tm = std::localtime(&now_c);

                char timeBuffer[100];
                std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", now_tm);

                formattedMessage = fmt::format("[{}] {}", timeBuffer, formattedMessage);
            }

            // Append stack trace if requested (platform-dependent, stubbed here)
            if (m_config.showStackTrace)
            {
                std::ostringstream oss;
                oss << std::stacktrace::current(); // Trick to get a readable stack trace

                formattedMessage += fmt::format("\n[Stack Trace]\n{}", oss.str());
            }

            for (const auto& [name, sink] : m_sinks)
            {
                switch (_level)
                {
                    case LogLevel::trace:
                        sink->trace(formattedMessage);
                        break;
                    case LogLevel::debug:
                        sink->debug(formattedMessage);
                        break;
                    case LogLevel::info:
                        sink->info(formattedMessage);
                        break;
                    case LogLevel::warn:
                        sink->warn(formattedMessage);
                        break;
                    case LogLevel::error:
                        sink->error(formattedMessage);
                        break;
                    case LogLevel::critical:
                        sink->critical(formattedMessage);
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

            m_history.at(tid).emplace_back(formattedMessage);
        }
        catch (const std::exception& e)
        {
            SystemConsole::printError(e.what());
        }
    }

    [[nodiscard]] static LoggerManager* getInstance() { return s_instance; }
};

} // namespace core
} // namespace mosaic

#if defined(MOSAIC_DEBUG_BUILD) || defined(MOSAIC_DEV_BUILD)
#define MOSAIC_TRACE(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::trace, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
#define MOSAIC_DEBUG(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::debug, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
#else
#define MOSAIC_TRACE(msg, ...) ((void)0)
#define MOSAIC_DEBUG(msg, ...) ((void)0)
#endif

#define MOSAIC_INFO(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::info, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
#define MOSAIC_WARN(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::warn, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
#define MOSAIC_ERROR(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::error, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
#define MOSAIC_CRITICAL(msg, ...)                                                     \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::critical, \
                                                    msg __VA_OPT__(, __VA_ARGS__))
