#include "saturn/exec/thread_pool.hpp"

#include <string>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>
#include <condition_variable>

#ifdef SATURN_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <concurrentqueue/moodycamel/concurrentqueue.h>

namespace saturn
{
namespace exec
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadPool::Impl
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ThreadPool::Impl
{
    static bool s_created;

    // fixed after initialization (no need for atomic)
    uint32_t workersCount = 0;

    moodycamel::ConcurrentQueue<MoveOnlyTask<void()>> globalTaskQueue;
    std::vector<std::unique_ptr<ThreadWorker>> workers;

    // Aligned to 64 bytes boundaries to avoid false sharing
    alignas(SATURN_CACHE_LINE_SIZE) std::atomic<uint32_t> idleWorkersCount;
    alignas(SATURN_CACHE_LINE_SIZE) std::atomic<bool> stop;

    Impl();
    ~Impl();
};

bool ThreadPool::Impl::s_created = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
// WorkerStats - Cache-line aligned per-worker statistics
////////////////////////////////////////////////////////////////////////////////////////////////////

struct alignas(SATURN_CACHE_LINE_SIZE) WorkerStats
{
    std::atomic<uint64_t> tasksExecuted{0};
    std::atomic<uint64_t> tasksStolen{0};
    std::atomic<uint64_t> stealAttempts{0};
    std::atomic<uint64_t> tasksReceivedFromGlobal{0};

    void reset() noexcept
    {
        tasksExecuted.store(0, std::memory_order_relaxed);
        tasksStolen.store(0, std::memory_order_relaxed);
        stealAttempts.store(0, std::memory_order_relaxed);
        tasksReceivedFromGlobal.store(0, std::memory_order_relaxed);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadWorker
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr auto k_idleTimeout = std::chrono::milliseconds(1);
constexpr size_t k_stealBatchSize = 8;
constexpr size_t k_maxPopCountFromGlobal = 16;

class ThreadWorker final
{
   private:
    ThreadPool::Impl* m_pool = nullptr;

    std::condition_variable m_cv;
    std::mutex m_cvMutex;

   public:
    uint32_t m_idx;
    size_t m_tid;

    std::string m_debugName;
    WorkerSharingMode m_sharingMode;

    std::jthread m_thread;

    moodycamel::ConcurrentQueue<MoveOnlyTask<void()>> m_taskQueue;

    WorkerStats m_stats;

   public:
    explicit ThreadWorker(uint32_t _idx, const std::string& _debugName, ThreadPool::Impl* _pool,
                          WorkerSharingMode _sharingMode = worker_sharing_presets::shared)
        : m_pool(_pool),
          m_idx(_idx),
          m_debugName(_debugName),
          m_sharingMode(_sharingMode),
          m_tid(0) {};

    void operator()()
    {
        auto& impl = *m_pool;

        m_tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        MoveOnlyTask<void()> task;

        while (!impl.stop.load(std::memory_order_acquire))
        {
            if (tryPopLocal(task) || tryPopGlobal(task) || tryStealing(task))
            {
                executeTask(task);
                continue;
            }

            waitForWork();
        }
    }

    void notify() { m_cv.notify_one(); }

    [[nodiscard]] size_t getTasksCount() const noexcept { return m_taskQueue.size_approx(); }

   private:
    void waitForWork()
    {
        auto& impl = *m_pool;

        std::unique_lock lock(m_cvMutex);

        impl.idleWorkersCount.fetch_add(1, std::memory_order_release);

        m_cv.wait_for(lock, k_idleTimeout,
                      [&]
                      {
                          return impl.stop.load(std::memory_order_acquire) ||
                                 m_taskQueue.size_approx() > 0 ||
                                 impl.globalTaskQueue.size_approx() > 0;
                      });

        impl.idleWorkersCount.fetch_sub(1, std::memory_order_release);
    }

    bool tryPopLocal(MoveOnlyTask<void()>& _outTask) noexcept
    {
        return m_taskQueue.try_dequeue(_outTask);
    }

    bool tryPopGlobal(MoveOnlyTask<void()>& _outTask) noexcept
    {
        if (!saturn::utils::hasFlag(m_sharingMode, WorkerSharingMode::global_consumer))
        {
            return false;
        }

        auto& impl = *m_pool;

        MoveOnlyTask<void()> tasks[k_maxPopCountFromGlobal];
        const size_t count = impl.globalTaskQueue.try_dequeue_bulk(tasks, k_maxPopCountFromGlobal);

        if (count == 0) return false;

        m_stats.tasksReceivedFromGlobal.fetch_add(count, std::memory_order_relaxed);

        _outTask = std::move(tasks[0]);

        for (size_t i = 1; i < count; ++i) m_taskQueue.enqueue(std::move(tasks[i]));

        return true;
    }

    bool tryStealing(MoveOnlyTask<void()>& _outTask) noexcept
    {
        const auto& impl = *m_pool;
        const auto& workers = impl.workers;
        const uint32_t n = impl.workersCount;

        if (n <= 1) return false;

        m_stats.stealAttempts.fetch_add(1, std::memory_order_relaxed);

        static thread_local uint32_t nextStart = 0;
        const uint32_t start = nextStart;
        nextStart = (nextStart + 1) % n;

        const auto thisWorker = this;

        for (uint32_t i = 0; i < n; ++i)
        {
            const uint32_t victimIdx = (start + i) % n;
            const auto victim = workers[victimIdx].get();

            if (victim == thisWorker) continue;
            if (!saturn::utils::hasFlag(victim->m_sharingMode, WorkerSharingMode::allow_steal))
            {
                continue;
            }

            MoveOnlyTask<void()> stolen[k_stealBatchSize];
            size_t actualCount = victim->m_taskQueue.try_dequeue_bulk(stolen, k_stealBatchSize);

            if (actualCount == 0) continue;

            m_stats.tasksStolen.fetch_add(actualCount, std::memory_order_relaxed);

            _outTask = std::move(stolen[0]);

            if (actualCount > 1)
            {
                for (size_t j = 1; j < actualCount; ++j)
                {
                    m_taskQueue.enqueue(std::move(stolen[j]));
                }
            }

            return true;
        }

        return false;
    }

    void executeTask(MoveOnlyTask<void()>& task) noexcept
    {
        try
        {
            task();
        }
        catch (const std::exception& e)
        {
            SATURN_ERROR("Worker {}: task threw std::exception: {}", m_idx, e.what());
        }
        catch (...)
        {
            SATURN_ERROR("Worker {}: task threw unknown exception.", m_idx);
        }

        m_stats.tasksExecuted.fetch_add(1, std::memory_order_relaxed);

        task = nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadPool::Impl (with complete type for ThreadWorker)
////////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPool::Impl::Impl() : workersCount(0), idleWorkersCount(0), stop(false)
{
    assert(!s_created && "ThreadPool already exists!");
    s_created = true;

    stop.store(false, std::memory_order_relaxed);
};

ThreadPool::Impl::~Impl()
{
    stop.store(true, std::memory_order_relaxed);

    s_created = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadPool
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr auto k_minThreads = 5;

ThreadPool* ThreadPool::g_instance = nullptr;

ThreadPool::ThreadPool() : m_impl(new Impl())
{
    if (!g_instance) g_instance = this;
}

ThreadPool::~ThreadPool()
{
    shutdown();

    delete m_impl;
    m_impl = nullptr;

    if (g_instance == this) g_instance = nullptr;
}

pieces::RefResult<ThreadPool, std::string> ThreadPool::initialize(
    const saturn::core::CPUInfo& _cpuInfo) noexcept
{
    int logical = static_cast<int>(_cpuInfo.logicalCores);

    // -1 to leave one thread for main flow
    uint32_t workersCount =
        static_cast<uint32_t>(std::max(logical - 1, static_cast<int>(k_minThreads)));

    m_impl->workersCount = workersCount;
    m_impl->workers.reserve(workersCount);

    for (uint32_t i = 0; i < workersCount; ++i) setupWorker(i);
    for (uint32_t i = 0; i < workersCount; ++i) startupWorker(i);

    return pieces::OkRef<ThreadPool, std::string>(*this);
}

void ThreadPool::shutdown() noexcept
{
    m_impl->stop.store(true, std::memory_order_relaxed);

    for (auto& worker : m_impl->workers) worker->notify();

    for (auto& worker : m_impl->workers)
    {
        if (worker->m_thread.joinable()) worker->m_thread.join();
    }

    m_impl->workers.clear();
}

void ThreadPool::setWorkerAffinity(uint32_t _workerId, size_t _cpuCoreId) noexcept
{
    ThreadWorker* worker = getWorkerByIdx(_workerId);

    if (!worker)
    {
        SATURN_ERROR("Cannot set affinity for non-existing worker with id {}.", _workerId);
        return;
    }

    auto nativeHandle = worker->m_thread.native_handle();
#ifdef SATURN_PLATFORM_WINDOWS
    HANDLE threadHandle = static_cast<HANDLE>(nativeHandle);
    DWORD_PTR affinityMask = 1ULL << (_cpuCoreId);

    SetThreadAffinityMask(threadHandle, affinityMask);
#elif defined(SATURN_PLATFORM_LINUX) || defined(SATURN_PLATFORM_MACOS)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(_cpuCoreId, &cpuset);

    pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
#endif
}

void ThreadPool::setWorkerSharingMode(uint32_t _workerId, WorkerSharingMode _sharingMode) noexcept
{
    ThreadWorker* worker = getWorkerByIdx(_workerId);

    if (!worker)
    {
        SATURN_ERROR("Cannot set sharing mode for non-existing worker with id {}.", _workerId);
        return;
    }

    static std::atomic<uint32_t> s_indirectAcceptingWorkers{0};

    bool currentlyAcceptsIndirect =
        saturn::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::accept_indirect);
    bool willAcceptIndirect =
        saturn::utils::hasFlag(_sharingMode, WorkerSharingMode::accept_indirect);

    if (currentlyAcceptsIndirect && !willAcceptIndirect)
    {
        s_indirectAcceptingWorkers.fetch_sub(1, std::memory_order_acq_rel);
    }
    else if (!currentlyAcceptsIndirect && willAcceptIndirect)
    {
        s_indirectAcceptingWorkers.fetch_add(1, std::memory_order_acq_rel);
    }

    if (s_indirectAcceptingWorkers.load(std::memory_order_acquire) == 0 && !willAcceptIndirect)
    {
        SATURN_WARN(
            "Cannot disable indirect submissions for worker {}: would cause system stall. At least "
            "one worker must accept indirect submissions.",
            _workerId);
        return;
    }

    worker->m_sharingMode = _sharingMode;
}

bool ThreadPool::isRunning() const noexcept
{
    return !m_impl->stop.load(std::memory_order_relaxed);
}

uint32_t ThreadPool::getWorkersCount() const noexcept { return m_impl->workersCount; }

uint32_t ThreadPool::getBusyWorkersCount() const noexcept
{
    return m_impl->workersCount - m_impl->idleWorkersCount.load(std::memory_order_acquire);
}

uint32_t ThreadPool::getIdleWorkersCount() const noexcept
{
    return m_impl->idleWorkersCount.load(std::memory_order_acquire);
}

WorkerStatsSnapshot ThreadPool::getWorkerStats(uint32_t _workerIdx) const noexcept
{
    if (_workerIdx >= m_impl->workersCount)
    {
        SATURN_ERROR("ThreadWorker with id {} does not exist.", _workerIdx);
        return {};
    }

    const auto& worker = m_impl->workers[_workerIdx];
    const auto& stats = worker->m_stats;

    return {
        stats.tasksExecuted.load(std::memory_order_relaxed),
        stats.tasksStolen.load(std::memory_order_relaxed),
        stats.stealAttempts.load(std::memory_order_relaxed),
        stats.tasksReceivedFromGlobal.load(std::memory_order_relaxed),
        worker->getTasksCount(),
    };
}

PoolStatsSnapshot ThreadPool::getPoolStats() const noexcept
{
    PoolStatsSnapshot result{};

    for (uint32_t i = 0; i < m_impl->workersCount; ++i)
    {
        const auto& worker = m_impl->workers[i];
        const auto& stats = worker->m_stats;

        result.totalTasksExecuted += stats.tasksExecuted.load(std::memory_order_relaxed);
        result.totalTasksStolen += stats.tasksStolen.load(std::memory_order_relaxed);
        result.totalStealAttempts += stats.stealAttempts.load(std::memory_order_relaxed);
        result.totalTasksReceivedFromGlobal +=
            stats.tasksReceivedFromGlobal.load(std::memory_order_relaxed);
        result.totalQueuedTasks += worker->getTasksCount();
    }

    return result;
}

void ThreadPool::resetStats() noexcept
{
    for (auto& worker : m_impl->workers)
    {
        worker->m_stats.reset();
    }
}

ThreadWorker* ThreadPool::getRandomWorker() const noexcept
{
    if (m_impl->workers.empty()) return nullptr;

    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist{
        0,
        static_cast<uint32_t>(m_impl->workersCount - 1),
    };

    uint32_t idx = dist(rng);

    return m_impl->workers[idx].get();
}

ThreadWorker* ThreadPool::getWorkerByIdx(uint32_t _idx) const noexcept
{
    if (_idx >= m_impl->workersCount)
    {
        SATURN_ERROR("ThreadWorker with id {} does not exist.", _idx);
        return nullptr;
    }

    return m_impl->workers[_idx].get();
}

ThreadWorker* ThreadPool::getWorkerByDebugName(const std::string& _debugName) const noexcept
{
    for (const auto& worker : m_impl->workers)
    {
        if (worker->m_debugName == _debugName) return worker.get();
    }

    SATURN_ERROR("ThreadWorker with debug name '{}' does not exist.", _debugName);

    return nullptr;
}

void ThreadPool::setupWorker(uint32_t _idx)
{
    std::string workerDebugName = "ThreadWorker-" + std::to_string(_idx);

    m_impl->workers.emplace_back(std::make_unique<ThreadWorker>(_idx, workerDebugName, m_impl));
}

void ThreadPool::startupWorker(uint32_t _idx)
{
    auto worker = m_impl->workers[_idx].get();

    worker->m_thread = std::jthread(&ThreadWorker::operator(), worker);

    setWorkerAffinity(_idx, _idx + 1); // +1 to leave core 0 for main thread
}

bool ThreadPool::assignTaskToGlobal(MoveOnlyTask<void()> _task) noexcept
{
    if (m_impl->stop.load(std::memory_order_acquire))
    {
        SATURN_WARN("ThreadPool is shutting down, cannot assign new tasks.");
        return false;
    }

    m_impl->globalTaskQueue.enqueue(std::move(_task));

    for (auto& worker : m_impl->workers)
    {
        if (saturn::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::global_consumer))
        {
            worker->notify();
        }
    }

    return true;
}

bool ThreadPool::assignTaskToWorker(MoveOnlyTask<void()> _task) noexcept
{
    if (m_impl->stop.load(std::memory_order_acquire))
    {
        SATURN_WARN("ThreadPool is shutting down, cannot assign new tasks.");
        return false;
    }

    for (uint32_t i = 0; i < m_impl->workersCount; ++i)
    {
        ThreadWorker* worker = getRandomWorker();

        if (saturn::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::accept_indirect))
        {
            worker->m_taskQueue.enqueue(std::move(_task));
            worker->notify();
            return true;
        }
    }

    SATURN_INFO("No thread worker available that accepts indirect task submissions.");

    m_impl->globalTaskQueue.enqueue(std::move(_task));

    for (auto& worker : m_impl->workers)
    {
        if (saturn::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::global_consumer))
        {
            worker->notify();
        }
    }

    return true;
}

bool ThreadPool::assignTaskToWorkerById(uint32_t _idx, MoveOnlyTask<void()> _task) noexcept
{
    ThreadWorker* worker = getWorkerByIdx(_idx);

    const WorkerSharingMode mode = worker->m_sharingMode;

    if (!saturn::utils::hasFlag(mode, WorkerSharingMode::accept_direct))
    {
        SATURN_ERROR("Worker with id {} does not accept direct task submissions.", _idx);
        return false;
    }

    worker->m_taskQueue.enqueue(std::move(_task));
    worker->notify();

    if (worker->m_taskQueue.size_approx() < k_stealBatchSize) return true;

    ThreadWorker* otherWorker = nullptr;

    while (true)
    {
        otherWorker = getRandomWorker();

        if (otherWorker != worker) break;
    }

    otherWorker->notify();

    return true;
}

bool ThreadPool::assignTaskToWorkerByDebugName(const std::string& _debugName,
                                               MoveOnlyTask<void()> _task) noexcept
{
    ThreadWorker* worker = getWorkerByDebugName(_debugName);

    const WorkerSharingMode mode = worker->m_sharingMode;

    if (!saturn::utils::hasFlag(mode, WorkerSharingMode::accept_direct))
    {
        SATURN_ERROR("Worker with debug name '{}' does not accept direct task submissions.",
                     _debugName);
        return false;
    }

    worker->m_taskQueue.enqueue(std::move(_task));
    worker->notify();

    if (worker->m_taskQueue.size_approx() < k_stealBatchSize) return true;

    ThreadWorker* otherWorker = nullptr;

    while (true)
    {
        otherWorker = getRandomWorker();

        if (otherWorker != worker) break;
    }

    otherWorker->notify();

    return true;
}

} // namespace exec
} // namespace saturn
