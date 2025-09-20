#include "mosaic/core/tracer.hpp"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
#include <nlohmann/json_fwd.hpp>

#include "mosaic/defines.hpp"
#include "mosaic/core/logger.hpp"
#include "mosaic/version.h"
#include "mosaic/core/cmd_line_parser.hpp"

namespace mosaic
{
namespace core
{

TracerManager* TracerManager::s_instance = nullptr;

ScopedTrace::ScopedTrace(const std::string& _name, TraceCategory _category) noexcept
    : m_valid(false)
{
    if (auto* manager = TracerManager::getInstance())
    {
        manager->beginTrace(_name, _category);
        m_valid = true;
    }
}

ScopedTrace::~ScopedTrace() noexcept
{
    if (!m_valid) return;

    if (auto* manager = TracerManager::getInstance()) manager->endTrace();
}

TracerManager::TracerManager(const TracerConfig& _config)
    : m_config(_config), m_fileCounter(0), m_nextTraceId(1)
{
    m_startTime = std::chrono::steady_clock::now();
    m_lastFlush = m_startTime;

    m_metadata = nlohmann::json::object();
    m_metadata["version"] = "1.0";
    m_metadata["engineVersion"] = MOSAIC_VERSION;

    // Store as time_t for nlohmann::json compatibility
    auto now = std::chrono::system_clock::now();
    auto duration = m_startTime - std::chrono::steady_clock::time_point();
    auto systemStartTime =
        now + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

    m_metadata["startTime"] = std::chrono::system_clock::to_time_t(systemStartTime);
    m_metadata["processId"] = 0;
    m_metadata["threadName"] = nlohmann::json::object();
    m_metadata["processName"] = CommandLineParser::getExecutableName();
}

bool TracerManager::initialize(const std::string& _tracesDir, const TracerConfig& _config) noexcept
{
    if (s_instance) return false;

    try
    {
        if (_tracesDir.empty()) return false;

        std::filesystem::path tracesDir(_tracesDir);
        if (!std::filesystem::exists(tracesDir))
        {
            if (!std::filesystem::create_directories(tracesDir)) return false;
        }

        s_instance = new TracerManager(_config);

        s_instance->m_tracesPath = _tracesDir;
        s_instance->m_currentFile = s_instance->generateFileName();

        std::ofstream testFile(s_instance->m_currentFile);
        if (!testFile.is_open())
        {
            delete s_instance;
            s_instance = nullptr;

            return false;
        }

        testFile.close();

        return true;
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());

        if (!s_instance)
        {
            delete s_instance;
            s_instance = nullptr;
        }

        return false;
    }
}

void TracerManager::shutdown() noexcept
{
    if (!s_instance) return;

    s_instance->flush();

    delete s_instance;
    s_instance = nullptr;
}

void TracerManager::beginTrace(const std::string& _name, TraceCategory _category,
                               const std::string& _args) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(_category, TracePhase::complete, _name, std::hash<std::thread::id>{}(tid), 0,
                    timestamp, 0, 0, _args);

        std::lock_guard<std::mutex> lock(m_tracesMutex);
        m_activeTraces[tid].push(trace);
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::endTrace() noexcept
{
    if (!m_config.enabled) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto endTime = getCurrentTimestamp();

        std::lock_guard<std::mutex> activeLock(m_tracesMutex);

        auto it = m_activeTraces.find(tid);
        if (it == m_activeTraces.end() || it->second.empty()) return;

        auto& trace = it->second.top();
        trace.duration = endTime - trace.timestamp;

        {
            std::lock_guard<std::mutex> completedLock(m_completedMutex);
            m_completedTraces.push_back(trace);

            if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
        }

        it->second.pop();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::instantTrace(const std::string& _name, TraceCategory _category,
                                 const std::string& _args) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(_category, TracePhase::instant, _name, std::hash<std::thread::id>{}(tid), 0,
                    timestamp, 0, 0, _args);

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::counterTrace(const std::string& _name, double _value,
                                 TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        nlohmann::json args = nlohmann::json::object();
        args[_name] = _value;

        Trace trace(_category, TracePhase::counter, _name, std::hash<std::thread::id>{}(tid), 0,
                    timestamp, 0, 0, args.dump());

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::metadataTrace(const std::string& _name, const std::string& _value,
                                  TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        nlohmann::json args = nlohmann::json::object();
        args["name"] = _value;

        Trace trace(_category, TracePhase::metadata, _name, std::hash<std::thread::id>{}(tid), 0,
                    timestamp, 0, 0, args.dump());

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::objectCreated(const std::string& _name, const std::string& _args,
                                  TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(_category, TracePhase::object_created, _name, std::hash<std::thread::id>{}(tid),
                    0, timestamp, 0, 0, _args);

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::objectSnapshot(const std::string& _name, const std::string& _args,
                                   TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(_category, TracePhase::object_snapshot, _name,
                    std::hash<std::thread::id>{}(tid), 0, timestamp, 0, 0, _args);

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::objectDestroyed(const std::string& _name, const std::string& _args,
                                    TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return;

    if (_name.empty()) return;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(_category, TracePhase::object_destroyed, _name,
                    std::hash<std::thread::id>{}(tid), 0, timestamp, 0, 0, _args);

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

uint64_t TracerManager::beginFlowTrace(const std::string& _name, TraceCategory _category) noexcept
{
    if (!m_config.enabled || !m_config.categoryEnabled[static_cast<int>(_category)]) return 0;

    if (_name.empty()) return 0;

    try
    {
        auto tid = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();
        auto flowId = m_nextTraceId++;

        Trace trace(_category, TracePhase::flow_begin, _name, std::hash<std::thread::id>{}(tid), 0,
                    timestamp, 0, flowId, "{}");

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();

        return flowId;
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
        return 0;
    }
}

void TracerManager::stepFlowTrace(uint64_t _flowId, const std::string& _name) noexcept
{
    if (!m_config.enabled || _flowId == 0) return;

    try
    {
        if (_name.empty()) return;

        auto threadId = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(TraceCategory::function, TracePhase::flow_step, _name,
                    std::hash<std::thread::id>{}(threadId), 0, timestamp, 0, _flowId, "{}");

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::endFlowTrace(uint64_t _flowId, const std::string& _name) noexcept
{
    if (!m_config.enabled || _flowId == 0) return;

    try
    {
        if (_name.empty()) return;

        auto threadId = std::this_thread::get_id();
        auto timestamp = getCurrentTimestamp();

        Trace trace(TraceCategory::function, TracePhase::flow_end, _name,
                    std::hash<std::thread::id>{}(threadId), 0, timestamp, 0, _flowId, "{}");

        std::lock_guard<std::mutex> lock(m_completedMutex);
        m_completedTraces.push_back(trace);

        if (m_config.autoFlush && m_completedTraces.size() >= m_config.maxTraces) flushToFile();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::flush() noexcept
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    flushToFile();
}

void TracerManager::clear() noexcept
{
    std::lock_guard<std::mutex> completedLock(m_completedMutex);
    std::lock_guard<std::mutex> traceLock(m_tracesMutex);

    m_completedTraces.clear();
    m_activeTraces.clear();
}

size_t TracerManager::getActiveTraceCount() const noexcept
{
    try
    {
        std::lock_guard<std::mutex> lock(m_tracesMutex);
        size_t count = 0;
        for (const auto& [tid, stack] : m_activeTraces)
        {
            count += stack.size();
        }
        return count;
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

size_t TracerManager::getCompletedTraceCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    return m_completedTraces.size();
}

double TracerManager::getTracingOverheadMs() const noexcept
{
    return 0.001; // 1 microsecond conservative estimate per trace
}

void TracerManager::flushToFile() noexcept
{
    if (m_completedTraces.empty()) return;

    try
    {
        if (std::filesystem::exists(m_currentFile))
        {
            auto fileSize = std::filesystem::file_size(m_currentFile);
            if (fileSize > MB_TO_BYTES(m_config.maxFileSizeMb)) rotateFile();
        }

        nlohmann::json output;
        output["traceEvents"] = nlohmann::json::array();

        for (const auto& trace : m_completedTraces)
        {
            output["traceEvents"].push_back(traceToJson(trace));
        }

        output["metadata"] = m_metadata;
        output["endTime"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::ofstream file(m_currentFile, std::ios::app);
        if (file.is_open())
        {
            file << output.dump(4) << std::endl;
            file.close();
        }

        m_completedTraces.clear();
        m_lastFlush = std::chrono::steady_clock::now();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
    }
}

void TracerManager::rotateFile() noexcept
{
    m_currentFile = generateFileName();
    m_fileCounter++;
}

std::string TracerManager::generateFileName() noexcept
{
    try
    {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << m_tracesPath << "/trace_" << timestamp;

        if (m_fileCounter > 0)
        {
            oss << "_" << std::setfill('0') << std::setw(3) << m_fileCounter;
        }

        oss << ".json";
        return oss.str();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
        return m_tracesPath + "/trace_fallback.json";
    }
}

nlohmann::json TracerManager::traceToJson(const Trace& _trace) const noexcept
{
    try
    {
        nlohmann::json json;
        json["cat"] = c_categoryNames[static_cast<int>(_trace.category)];
        json["ph"] = c_phaseNames[static_cast<int>(_trace.phase)];
        json["name"] = _trace.name;
        json["pid"] = _trace.pid;
        json["tid"] = _trace.tid;
        json["ts"] = _trace.timestamp;

        if (_trace.duration > 0) json["dur"] = _trace.duration;

        if (_trace.id > 0) json["id"] = _trace.id;

        if (!_trace.args.empty() && _trace.args != "{}")
        {
            try
            {
                json["args"] = nlohmann::json::parse(_trace.args);
            }
            catch (const std::exception&)
            {
                json["args"] = nlohmann::json::object();
            }
        }

        return json;
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR(e.what());
        return nlohmann::json::object();
    }
}

int64_t TracerManager::getCurrentTimestamp() const noexcept
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
}

} // namespace core
} // namespace mosaic
