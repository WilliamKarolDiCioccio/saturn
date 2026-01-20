#pragma once

#include "saturn/defines.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

SATURN_DISABLE_ALL_WARNINGS
#include <nlohmann/json.hpp>
SATURN_POP_WARNINGS

#include "logger.hpp"

namespace saturn
{
namespace tools
{

enum class TraceCategory
{
    function = 0,
    scope = 1,
    gpu = 2,
    io = 3,
    memory = 4,
    render = 5,
    network = 6,
    custom = 7
};

enum class TracePhase
{
    complete = 0,         // 'X' - Complete events (duration events)
    begin = 1,            // 'B' - Begin events
    end = 2,              // 'E' - End events
    instant = 3,          // 'i' - Instant events
    counter = 4,          // 'C' - Counter events
    metadata = 5,         // 'M' - Metadata events
    sample = 6,           // 'P' - Sample events
    object_created = 7,   // 'N' - Object created
    object_snapshot = 8,  // 'O' - Object snapshot
    object_destroyed = 9, // 'D' - Object destroyed
    flow_begin = 10,      // 's' - Flow start
    flow_step = 11,       // 't' - Flow step
    flow_end = 12         // 'f' - Flow end
};

struct Trace
{
    TraceCategory category;
    TracePhase phase;
    std::string name;
    std::string args; // JSON string for additional arguments
    size_t tid;
    uint32_t pid;
    int64_t timestamp;
    int64_t duration;
    uint64_t id; // For flow events and object tracking

    Trace() = default;

    Trace(TraceCategory _category, TracePhase _phase, const std::string& _name, size_t _tid,
          uint32_t _pid, int64_t _timestamp, int64_t _duration = 0, uint64_t _id = 0,
          const std::string& _args = "{}")
        : category(_category),
          phase(_phase),
          name(_name),
          args(_args),
          tid(_tid),
          pid(_pid),
          timestamp(_timestamp),
          duration(_duration),
          id(_id) {};
};

/**
 * @brief Lookup table for trace categories.
 */
constexpr std::array<const char*, 8> c_categoryNames = {
    "function", "scope", "gpu", "io", "memory", "render", "network", "custom",
};

/**
 * @brief Lookup table for trace phases.
 */
constexpr std::array<const char*, 13> c_phaseNames = {
    "X", "B", "E", "i", "C", "M", "P", "N", "O", "D", "s", "t", "f",
};

/**
 * @brief RAII helper for automatic trace scoping.
 */
class SATURN_API ScopedTrace final
{
   private:
    bool m_valid;

   public:
    ScopedTrace(const std::string& _name, TraceCategory _category = TraceCategory::scope) noexcept;
    ~ScopedTrace() noexcept;

    ScopedTrace(const ScopedTrace&) = delete;
    ScopedTrace& operator=(const ScopedTrace&) = delete;
    ScopedTrace(ScopedTrace&&) = delete;
    ScopedTrace& operator=(ScopedTrace&&) = delete;
};

/**
 * @brief Manages tracing functionality, including trace storage, metadata, and configuration.
 */
class SATURN_API Tracer final
{
   public:
    /**
     * @brief Configuration for the Tracer.
     *
     * This struct holds the configuration options for the tracer, including
     * whether tracing is enabled, auto-flush settings, flush intervals, maximum
     * number of traces, maximum file size, and category-specific enable flags.
     */
    struct Config
    {
        std::atomic_bool enabled;
        std::atomic_bool autoFlush;
        std::atomic_uint32_t flushIntervalMs;
        std::atomic_uint32_t maxTraces;
        std::atomic_uint32_t maxFileSizeMb; // MB

        std::array<std::atomic_bool, 8> categoryEnabled;

        Config(bool _enabled = true, bool _autoFlush = true, uint32_t _flushIntervalMs = 1000,
               uint32_t _maxTraces = 10000, uint32_t _maxFileSizeMb = 100)
            : enabled(_enabled),
              autoFlush(_autoFlush),
              flushIntervalMs(_flushIntervalMs),
              maxTraces(_maxTraces),
              maxFileSizeMb(_maxFileSizeMb)
        {
            for (auto& e : categoryEnabled) e.store(true);
        }

        Config& operator=(const Config& other)
        {
            if (this == &other) return *this;

            enabled.store(other.enabled.load());
            autoFlush.store(other.autoFlush.load());
            flushIntervalMs.store(other.flushIntervalMs.load());
            maxTraces.store(other.maxTraces.load());
            maxFileSizeMb.store(other.maxFileSizeMb.load());

            for (size_t i = 0; i < categoryEnabled.size(); ++i)
            {
                categoryEnabled[i].store(other.categoryEnabled[i].load());
            }

            return *this;
        }
    };

   private:
    static Tracer* s_instance;

    // Actual trace storage

    std::unordered_map<std::thread::id, std::stack<Trace>> m_activeTraces;
    mutable std::mutex m_tracesMutex;
    std::vector<Trace> m_completedTraces;
    mutable std::mutex m_completedMutex;

    // Metadata for the trace session

    nlohmann::json m_metadata;
    std::string m_currentFile;
    uint32_t m_fileCounter;

    // Timing and ID management

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastFlush;
    uint64_t m_nextTraceId;

    Config m_config;

   private:
    Tracer() : m_fileCounter(0), m_nextTraceId(1) {};
    ~Tracer() = default;

   public:
    static bool initialize(const Config& _config = Config()) noexcept;
    static void shutdown() noexcept;

    // Configurable options

    void setEnabled(bool _enabled) noexcept { m_config.enabled = _enabled; }
    [[nodiscard]] bool isEnabled() const noexcept { return m_config.enabled; }

    void setAutoFlush(bool _autoFlush) noexcept { m_config.autoFlush = _autoFlush; }
    [[nodiscard]] bool isAutoFlush() const noexcept { return m_config.autoFlush; }

    void setflushIntervalMs(uint32_t _interval) noexcept { m_config.flushIntervalMs = _interval; }
    [[nodiscard]] uint32_t getflushIntervalMs() const noexcept { return m_config.flushIntervalMs; }

    void setMaxTraces(uint32_t _maxTraces) noexcept { m_config.maxTraces = _maxTraces; }
    [[nodiscard]] uint32_t getMaxTraces() const noexcept { return m_config.maxTraces; }

    void enableCategory(TraceCategory _category, bool _enabled) noexcept
    {
        m_config.categoryEnabled[static_cast<int>(_category)] = _enabled;
    }

    [[nodiscard]] bool isCategoryEnabled(TraceCategory _category) const noexcept
    {
        return m_config.categoryEnabled[static_cast<int>(_category)];
    }

    // Tracing methods

    void beginTrace(const std::string& _name, TraceCategory _category = TraceCategory::function,
                    const std::string& _args = "{}") noexcept;
    void endTrace() noexcept;

    void instantTrace(const std::string& _name, TraceCategory _category = TraceCategory::function,
                      const std::string& _args = "{}") noexcept;

    void counterTrace(const std::string& _name, double _value,
                      TraceCategory _category = TraceCategory::function) noexcept;

    void metadataTrace(const std::string& _name, const std::string& _value,
                       TraceCategory _category = TraceCategory::function) noexcept;

    void objectCreated(const std::string& _name, const std::string& _args = "{}",
                       TraceCategory _category = TraceCategory::function) noexcept;
    void objectSnapshot(const std::string& _name, const std::string& _args = "{}",
                        TraceCategory _category = TraceCategory::function) noexcept;
    void objectDestroyed(const std::string& _name, const std::string& _args = "{}",
                         TraceCategory _category = TraceCategory::function) noexcept;

    uint64_t beginFlowTrace(const std::string& _name,
                            TraceCategory _category = TraceCategory::function) noexcept;
    void stepFlowTrace(uint64_t _flowId, const std::string& _name) noexcept;
    void endFlowTrace(uint64_t _flowId, const std::string& _name) noexcept;

    // Manual flush control

    void flush() noexcept;
    void clear() noexcept;

    // Statistics

    [[nodiscard]] size_t getActiveTraceCount() const noexcept;
    [[nodiscard]] size_t getCompletedTraceCount() const noexcept;
    [[nodiscard]] double getTracingOverheadMs() const noexcept;

    [[nodiscard]] static Tracer* getInstance() noexcept { return s_instance; }

   private:
    void flushToFile() noexcept;
    void rotateFile() noexcept;
    std::string generateFileName() noexcept;
    nlohmann::json traceToJson(const Trace& _trace) const noexcept;
    int64_t getCurrentTimestamp() const noexcept;
};

} // namespace tools
} // namespace saturn

#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)

#define SATURN_TRACE_FUNCTION() \
    saturn::tools::ScopedTrace _trace(__FUNCTION__, saturn::tools::TraceCategory::function)

#define SATURN_TRACE_SCOPE(_Name) \
    saturn::tools::ScopedTrace _trace(_Name, saturn::tools::TraceCategory::scope)

#define SATURN_TRACE_BEGIN(_Name, _Category) \
    saturn::tools::Tracer::getInstance()->beginTrace(_Name, _Category)

#define SATURN_TRACE_END() saturn::tools::Tracer::getInstance()->endTrace()

#define SATURN_TRACE_INSTANT(_Name, _Category) \
    saturn::tools::Tracer::getInstance()->instantTrace(_Name, _Category)

#define SATURN_TRACE_COUNTER(_Name, _Value) \
    saturn::tools::Tracer::getInstance()->counterTrace(_Name, _Value)

#define SATURN_TRACE_METADATA(_Name, _Value) \
    saturn::tools::Tracer::getInstance()->metadataTrace(_Name, _Value)

#define SATURN_TRACE_OBJECT_CREATED(_Name, _Args) \
    saturn::tools::Tracer::getInstance()->objectCreated(_Name, _Args)
#define SATURN_TRACE_OBJECT_SNAPSHOT(_Name, _Args) \
    saturn::tools::Tracer::getInstance()->objectSnapshot(_Name, _Args)
#define SATURN_TRACE_OBJECT_DESTROYED(_Name, _Args) \
    saturn::tools::Tracer::getInstance()->objectDestroyed(_Name, _Args)

#define SATURN_TRACE_FLOW_BEGIN(_Name, _Category) \
    saturn::tools::Tracer::getInstance()->beginFlowTrace(_Name, _Category)
#define SATURN_TRACE_FLOW_STEP(_FlowID, _Name) \
    saturn::tools::Tracer::getInstance()->stepFlowTrace(_FlowID, _Name)
#define SATURN_TRACE_FLOW_END(_FlowID, _Name) \
    saturn::tools::Tracer::getInstance()->endFlowTrace(_FlowID, _Name)

#else

#define SATURN_TRACE_FUNCTION() ((void)0)
#define SATURN_TRACE_SCOPE(name) ((void)0)
#define SATURN_TRACE_BEGIN(_Name, _Category) ((void)0)
#define SATURN_TRACE_END() ((void)0)
#define SATURN_TRACE_INSTANT(_Name, _Category) ((void)0)
#define SATURN_TRACE_COUNTER(_Name, _Value) ((void)0)
#define SATURN_TRACE_METADATA(_Name, _Value) ((void)0)
#define SATURN_TRACE_OBJECT_CREATED(_Name, _Args) ((void)0)
#define SATURN_TRACE_OBJECT_SNAPSHOT(_Name, _Args) ((void)0)
#define SATURN_TRACE_OBJECT_DESTROYED(_Name, _Args) ((void)0)
#define SATURN_TRACE_FLOW_BEGIN(_Name, _Category) 0
#define SATURN_TRACE_FLOW_STEP(_FlowID, _Name) ((void)0)
#define SATURN_TRACE_FLOW_END(_FlowID, _Name) ((void)0)

#endif
