#pragma once

#include "saturn/defines.hpp"

#include <memory>
#include <string>
#include <array>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

#include <fmt/format.h>

#include <pieces/core/result.hpp>

#include "saturn/core/sys_console.hpp"

namespace saturn
{
namespace tools
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
 * @brief Manages logging functionality, including sink management, history, and configuration.
 */
class Logger final
{
   public:
    /**
     * @brief Configuration for the Logger.
     *
     * This struct allows customization of logging behavior, such as enabling/disabling log levels,
     * showing thread IDs, timestamps, and stack traces, as well as setting the history size
     * and base logs path.
     */
    struct Config
    {
        std::atomic_bool showLevel;
        std::atomic_bool showTid;
        std::atomic_bool showTimestamp;
        std::atomic_bool showStackTrace;

        std::atomic_uint16_t historySize;

        std::array<std::atomic_bool, 6> levelEnabled;

        Config(bool _showLevel = true, bool _showTid = false, bool _showTimestamp = true,
               bool _showStackTrace = false, uint16_t _historySize = k_logHistorySize)
            : showLevel(_showLevel),
              showTid(_showTid),
              showTimestamp(_showTimestamp),
              showStackTrace(_showStackTrace),
              historySize(_historySize)
        {
            for (auto& e : levelEnabled) e.store(true);
        }

        Config& operator=(const Config& other)
        {
            if (this == &other) return *this;

            showLevel.store(other.showLevel.load());
            showTid.store(other.showTid.load());
            showTimestamp.store(other.showTimestamp.load());
            showStackTrace.store(other.showStackTrace.load());
            historySize.store(other.historySize.load());

            for (size_t i = 0; i < levelEnabled.size(); ++i)
            {
                levelEnabled[i].store(other.levelEnabled[i].load());
            }

            return *this;
        }
    };

   private:
    SATURN_API static Logger* s_instance;

    std::unordered_map<std::string, std::shared_ptr<Sink>> m_sinks;
    std::mutex m_sinksMutex;
    std::unordered_map<std::thread::id, std::vector<std::string>> m_history;
    std::mutex m_historyMutex;

    Config m_config;

   private:
    Logger() = default;
    ~Logger() = default;

   public:
    SATURN_API static bool initialize(const Config& _config = Config()) noexcept;
    SATURN_API static void shutdown() noexcept;

    // Configurable options

    inline void setShowLevel(bool _show) noexcept { m_config.showLevel = _show; }
    [[nodiscard]] inline bool isShowLevel() const noexcept { return m_config.showLevel; }

    inline void setShowTid(bool _show) noexcept { m_config.showTid = _show; }
    [[nodiscard]] inline bool isShowTid() const noexcept { return m_config.showTid; }

    inline void setShowTimestamp(bool _show) noexcept { m_config.showTimestamp = _show; }
    [[nodiscard]] inline bool isShowTimestamp() const noexcept { return m_config.showTimestamp; }

    inline void setShowStackTrace(bool _show) noexcept { m_config.showStackTrace = _show; }
    [[nodiscard]] inline bool isShowStackTrace() const noexcept { return m_config.showStackTrace; }

    inline void setHistorySize(uint16_t _size) noexcept
    {
        m_config.historySize = _size;
        m_history.reserve(_size);
    }

    [[nodiscard]] inline uint16_t getHistorySize() const noexcept { return m_config.historySize; }

    inline void enableLevel(LogLevel _level, bool _enabled) noexcept
    {
        m_config.levelEnabled[static_cast<int>(_level)] = _enabled;
    }

    [[nodiscard]] inline bool isLevelEnabled(LogLevel _level) const noexcept
    {
        return m_config.levelEnabled[static_cast<int>(_level)];
    }

    // Sink management

    template <typename SinkType>
    inline bool addSink(const std::string& _name, SinkType&& _sink) noexcept
        requires IsSink<SinkType>
    {
        if (m_sinks.find(_name) != m_sinks.end()) return false;

        std::lock_guard<std::mutex> lock(m_sinksMutex);

        auto sink = std::make_shared<SinkType>(std::forward<SinkType>(_sink));

        auto result = sink->initialize();

        if (result.isErr())
        {
            core::SystemConsole::printError(
                fmt::format("Failed to initialize sink '{}': {}", _name, result.error()));
            return false;
        }

        m_sinks.insert({_name, sink});

        return true;
    }

    inline void removeSink(const std::string& _name) noexcept
    {
        if (m_sinks.find(_name) == m_sinks.end()) return;

        std::lock_guard<std::mutex> lock(m_sinksMutex);

        m_sinks.at(_name)->shutdown();

        m_sinks.erase(_name);
    }

    inline void clearSinks() noexcept
    {
        std::lock_guard<std::mutex> lock(m_sinksMutex);

        m_sinks.clear();
    }

    // History management

    [[nodiscard]] inline const std::vector<std::string> getHistory() const noexcept
    {
        auto tid = std::this_thread::get_id();

        if (m_history.find(tid) == m_history.end()) return {};

        return m_history.at(tid);
    }

    inline void clearHistory() noexcept { m_history[std::this_thread::get_id()].clear(); }

    // Logging methods

    template <typename... Args>
    inline void log(LogLevel _level, const std::string& _message, Args&&... _args) noexcept
    {
        try
        {
            if (!s_instance) throw std::runtime_error("Logger is not initialized");

            if (!m_config.levelEnabled[static_cast<int>(_level)]) return;

            // Format message with arguments
            std::string formattedMessage = fmt::vformat(_message, fmt::make_format_args(_args...));

            logInternal(_level, formattedMessage);
        }
        catch (const std::exception& e)
        {
            core::SystemConsole::printError(e.what());
        }
    }

    [[nodiscard]] static inline Logger* getInstance() { return s_instance; }

   private:
    SATURN_API void logInternal(LogLevel _level, std::string _message) noexcept;
};

} // namespace tools
} // namespace saturn

#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
#define SATURN_TRACE(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::trace, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
#define SATURN_DEBUG(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::debug, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
#else
#define SATURN_TRACE(_Msg, ...) ((void)0)
#define SATURN_DEBUG(_Msg, ...) ((void)0)
#endif

#define SATURN_INFO(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::info, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
#define SATURN_WARN(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::warn, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
#define SATURN_ERROR(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::error, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
#define SATURN_CRITICAL(_Msg, ...)                                               \
    saturn::tools::Logger::getInstance()->log(saturn::tools::LogLevel::critical, \
                                              _Msg __VA_OPT__(, __VA_ARGS__))
