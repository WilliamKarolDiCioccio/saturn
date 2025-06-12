#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>

#include "mosaic/defines.hpp"

MOSAIC_DISABLE_ALL_WARNINGS
#include <nlohmann/json.hpp>
MOSAIC_POP_WARNINGS

namespace mosaic
{
namespace core
{

enum class TraceCategory
{
    function,
    scope,
};

struct Trace
{
    TraceCategory category;
    std::string name;
    size_t threadID;
    int64_t start, end;
};

class MOSAIC_API TracerManager final
{
   private:
    static bool s_isInitialized;

    static std::array<std::string, 2> m_categories;
    static nlohmann::json m_data;
    static std::string m_tracesPath;
    static std::stack<Trace> m_activeTraces;
    static std::mutex m_mutex;

   private:
    TracerManager() = delete;

    static void ensureDirectoryExists(const std::string& path);

   public:
    static bool initialize(const std::string& _tracesDir) noexcept;
    static void shutdown() noexcept;
    static void beginTrace(const std::string& _name, TraceCategory _category) noexcept;
    static void endTrace() noexcept;

    static std::shared_ptr<TracerManager> get();
};

} // namespace core
} // namespace mosaic

#ifdef _DEBUG
#define MOSAIC_BEGIN_TRACE(name, category) mosaic::core::TracerManager::beginTrace(name, category)
#define MOSAIC_END_TRACE() mosaic::core::TracerManager::endTrace()
#else
#define MOSAIC_BEGIN_TRACE(name, category) ((void)0)
#define MOSAIC_END_TRACE() ((void)0)
#endif
