#include "saturn/tools/tracer.hpp"

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

#include "saturn/defines.hpp"
#include "saturn/tools/logger.hpp"
#include "saturn/version.h"
#include "saturn/core/cmd_line_parser.hpp"

namespace saturn
{
namespace tools
{

Tracer* Tracer::s_instance = nullptr;

ScopedTrace::ScopedTrace(const std::string& _name, TraceCategory _category) noexcept
    : m_valid(false)
{
    if (auto* manager = Tracer::getInstance())
    {
        manager->beginTrace(_name, _category);
        m_valid = true;
    }
}

ScopedTrace::~ScopedTrace() noexcept
{
    if (!m_valid) return;

    if (auto* manager = Tracer::getInstance()) manager->endTrace();
}

bool Tracer::initialize(const Config& _config) noexcept
{
    assert(s_instance == nullptr && "Tracer already exists!");
    s_instance = new Tracer();

    auto& instance = *s_instance;

    try
    {
        std::filesystem::path tracesDir("./traces");
        if (!std::filesystem::exists(tracesDir))
        {
            if (!std::filesystem::create_directories(tracesDir)) return false;
        }

        // Store configuration

        instance.m_config = _config;
        instance.m_startTime = std::chrono::steady_clock::now();
        instance.m_lastFlush = instance.m_startTime;
        instance.m_currentFile = instance.generateFileName();

        // Initialize traces with metadata

        auto& metadata = instance.m_metadata;

        metadata = nlohmann::json::object();
        metadata["version"] = "1.0";
        metadata["engineVersion"] = SATURN_VERSION;

        auto duration = instance.m_startTime - std::chrono::steady_clock::time_point();
        auto systemStartTime =
            std::chrono::system_clock::now() +
            std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

        metadata["startTime"] = std::chrono::system_clock::to_time_t(systemStartTime);
        metadata["processId"] = 0;
        metadata["threadName"] = nlohmann::json::object();
        metadata["processName"] = core::CommandLineParser::getInstance()->getExecutableName();

        std::ofstream testFile(instance.m_currentFile);
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
        SATURN_ERROR(e.what());
        return false;
    }
}

void Tracer::shutdown() noexcept
{
    assert(s_instance != nullptr && "Tracer does not exist!");

    auto& instance = *s_instance;

    instance.flush();

    delete s_instance;
    s_instance = nullptr;
}

void Tracer::beginTrace(const std::string& _name, TraceCategory _category,
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::endTrace() noexcept
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::instantTrace(const std::string& _name, TraceCategory _category,
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::counterTrace(const std::string& _name, double _value, TraceCategory _category) noexcept
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::metadataTrace(const std::string& _name, const std::string& _value,
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::objectCreated(const std::string& _name, const std::string& _args,
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::objectSnapshot(const std::string& _name, const std::string& _args,
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::objectDestroyed(const std::string& _name, const std::string& _args,
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
        SATURN_ERROR(e.what());
    }
}

uint64_t Tracer::beginFlowTrace(const std::string& _name, TraceCategory _category) noexcept
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
        SATURN_ERROR(e.what());
        return 0;
    }
}

void Tracer::stepFlowTrace(uint64_t _flowId, const std::string& _name) noexcept
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::endFlowTrace(uint64_t _flowId, const std::string& _name) noexcept
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::flush() noexcept
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    flushToFile();
}

void Tracer::clear() noexcept
{
    std::lock_guard<std::mutex> completedLock(m_completedMutex);
    std::lock_guard<std::mutex> traceLock(m_tracesMutex);

    m_completedTraces.clear();
    m_activeTraces.clear();
}

size_t Tracer::getActiveTraceCount() const noexcept
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

size_t Tracer::getCompletedTraceCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    return m_completedTraces.size();
}

double Tracer::getTracingOverheadMs() const noexcept
{
    return 0.001; // 1 microsecond conservative estimate per trace
}

void Tracer::flushToFile() noexcept
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
        SATURN_ERROR(e.what());
    }
}

void Tracer::rotateFile() noexcept
{
    m_currentFile = generateFileName();
    m_fileCounter++;
}

std::string Tracer::generateFileName() noexcept
{
    try
    {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << "./traces/trace_" << timestamp;

        if (m_fileCounter > 0)
        {
            oss << "_" << std::setfill('0') << std::setw(3) << m_fileCounter;
        }

        oss << ".json";
        return oss.str();
    }
    catch (const std::exception& e)
    {
        SATURN_ERROR(e.what());
        return "./traces/trace_fallback.json";
    }
}

nlohmann::json Tracer::traceToJson(const Trace& _trace) const noexcept
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
        SATURN_ERROR(e.what());
        return nlohmann::json::object();
    }
}

int64_t Tracer::getCurrentTimestamp() const noexcept
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
}

} // namespace tools
} // namespace saturn
