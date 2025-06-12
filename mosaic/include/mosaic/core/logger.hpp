#pragma once

#include "mosaic/defines.hpp"

#include <memory>
#include <string>
#include <array>
#include <chrono>
#include <unordered_map>

#include <fmt/format.h>

#include <pieces/result.hpp>

namespace mosaic
{
namespace core
{

constexpr uint16_t MOSAIC_LOG_HISTORY_SIZE = 1024;
constexpr const char* MOSAIC_LOGS_BASE_PATH = "logs/";

class Sink;

template <typename SinkType>
concept IsSink = std::derived_from<SinkType, Sink> && !std::is_abstract_v<SinkType>;

class Sink
{
   public:
    virtual ~Sink() = default;

    virtual void trace(const std::string& _message) const = 0;
    virtual void debug(const std::string& _message) const = 0;
    virtual void info(const std::string& _message) const = 0;
    virtual void warn(const std::string& _message) const = 0;
    virtual void error(const std::string& _message) const = 0;
    virtual void critical(const std::string& _message) const = 0;
};

class MOSAIC_API DefaultSink final : public Sink
{
   public:
    ~DefaultSink() override = default;

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

constexpr std::array<const char*, 6> c_levelNames = {
    "Trace", "Debug", "Info", "Warn", "Error", "Critical",
};

struct LoggerConfig
{
    bool showLevel;
    bool showTimestamp;
    bool showStackTrace;

    uint16_t historySize;
    std::string baseLogsPath;

    std::array<bool, 6> levelEnabled;

    LoggerConfig()
        : showLevel(true),
          showTimestamp(true),
          showStackTrace(false),
          historySize(MOSAIC_LOG_HISTORY_SIZE),
          baseLogsPath(MOSAIC_LOGS_BASE_PATH)
    {
        levelEnabled.fill(true);
    }
};

class MOSAIC_API LoggerManager final
{
   private:
    static LoggerManager* s_instance;

    std::unordered_map<std::string, std::shared_ptr<Sink>> m_sinks;
    std::vector<std::string> m_history;

    LoggerConfig m_config;

   public:
    LoggerManager(const LoggerConfig& _config);

   public:
    static bool initialize(const LoggerConfig& _config = LoggerConfig()) noexcept;
    static void shutdown() noexcept;

    // Configurable options

    void setShowLevel(bool _show) noexcept { m_config.showLevel = _show; }
    [[nodiscard]] bool isShowLevel() const noexcept { return m_config.showLevel; }

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

    void setBaseLogsPath(const std::string& _path) noexcept { m_config.baseLogsPath = _path; }

    [[nodiscard]] const std::string& getBaseLogsPath() const noexcept
    {
        return m_config.baseLogsPath;
    }

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
        m_sinks.insert({_name, std::make_shared<SinkType>(std::forward<SinkType>(_sink))});

        return true;
    }

    void removeSink(const std::string& _name) noexcept { m_sinks.erase(_name); }

    void clearSinks() noexcept { m_sinks.clear(); }

    // History management

    [[nodiscard]] const std::vector<std::string>& getHistory() const noexcept { return m_history; }

    void clearHistory() noexcept { m_history.clear(); }

    // Logging methods

    template <typename... Args>
    void log(LogLevel _level, const std::string& _message, Args&&... _args) noexcept
    {
        try
        {
            if (!m_config.levelEnabled[static_cast<int>(_level)]) return;

            // Format message with arguments
            std::string formattedMessage = fmt::vformat(_message, fmt::make_format_args(_args...));

            // Prepend log level if requested
            if (m_config.showLevel)
            {
                formattedMessage = fmt::format("[{}] {}", c_levelNames[static_cast<int>(_level)],
                                               formattedMessage);
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
                formattedMessage += "\n[stacktrace]\n";
                formattedMessage += "  (stack trace not implemented yet)";
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

            if (m_history.size() >= m_config.historySize) clearHistory();

            m_history.emplace_back(formattedMessage);
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Logging failure: %s\n", e.what());
        }
    }

    [[nodiscard]] static LoggerManager* getInstance() { return s_instance; }
};

} // namespace core
} // namespace mosaic

#ifdef MOSAIC_DEBUG_BUILD
#define MOSAIC_TRACE(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::trace, msg, \
                                                    ##__VA_ARGS__)
#define MOSAIC_DEBUG(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::debug, msg, \
                                                    ##__VA_ARGS__)
#else
#define MOSAIC_TRACE(msg, ...) ((void)0)
#define MOSAIC_DEBUG(msg, ...) ((void)0)
#endif

#define MOSAIC_INFO(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::info, msg, \
                                                    ##__VA_ARGS__)
#define MOSAIC_WARN(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::warn, msg, \
                                                    ##__VA_ARGS__)
#define MOSAIC_ERROR(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::error, msg, \
                                                    ##__VA_ARGS__)
#define MOSAIC_CRITICAL(msg, ...)                                                          \
    mosaic::core::LoggerManager::getInstance()->log(mosaic::core::LogLevel::critical, msg, \
                                                    ##__VA_ARGS__)
