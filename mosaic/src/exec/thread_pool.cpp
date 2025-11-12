#include "mosaic/exec/thread_pool.hpp"

#include <string>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>
#include <functional>
#include <condition_variable>

#ifdef MOSAIC_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <concurrentqueue/concurrentqueue.h>

#include "mosaic/core/sys_info.hpp"

namespace mosaic
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

    moodycamel::ConcurrentQueue<std::move_only_function<void()>> globalTaskQueue;
    std::vector<std::unique_ptr<ThreadWorker>> workers;

    // Aligned to 64 bytes boundaries to avoid false sharing
    alignas(MOSAIC_CACHE_LINE_SIZE) std::atomic<uint32_t> idleWorkersCount;
    alignas(MOSAIC_CACHE_LINE_SIZE) std::atomic<bool> stop;

    Impl() : workersCount(0), idleWorkersCount(0), stop(false)
    {
        assert(!s_created && "ThreadPool already exists!");
        s_created = true;

        stop.store(false, std::memory_order_relaxed);
    };

    ~Impl()
    {
        stop.store(true, std::memory_order_relaxed);

        s_created = false;
    }
};

bool ThreadPool::Impl::s_created = false;

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

    moodycamel::ConcurrentQueue<std::move_only_function<void()>> m_taskQueue;

   public:
    explicit ThreadWorker(uint32_t _idx, const std::string& _debugName, ThreadPool::Impl* _pool,
                          WorkerSharingMode _sharingMode = worker_sharing_presets::shared)
        : m_pool(_pool),
          m_idx(_idx),
          m_debugName(_debugName),
          m_sharingMode(_sharingMode),
          m_tid(0){};

    void operator()()
    {
        auto& impl = *m_pool;

        m_tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        std::move_only_function<void()> task;

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

    bool tryPopLocal(std::move_only_function<void()>& _outTask) noexcept
    {
        return m_taskQueue.try_dequeue(_outTask);
    }

    bool tryPopGlobal(std::move_only_function<void()>& _outTask) noexcept
    {
        if (!mosaic::utils::hasFlag(m_sharingMode, WorkerSharingMode::global_consumer))
        {
            return false;
        }

        auto& impl = *m_pool;

        std::move_only_function<void()> tasks[k_maxPopCountFromGlobal];
        const size_t count = impl.globalTaskQueue.try_dequeue_bulk(tasks, k_maxPopCountFromGlobal);

        if (count == 0) return false;

        _outTask = std::move(tasks[0]);

        for (size_t i = 1; i < count; ++i) m_taskQueue.enqueue(std::move(tasks[i]));

        return true;
    }

    bool tryStealing(std::move_only_function<void()>& _outTask) noexcept
    {
        const auto& impl = *m_pool;
        const auto& workers = impl.workers;
        const uint32_t n = impl.workersCount;

        if (n <= 1) return false;

        static thread_local uint32_t nextStart = 0;
        const uint32_t start = nextStart;
        nextStart = (nextStart + 1) % n;

        const auto thisWorker = this;

        for (uint32_t i = 0; i < n; ++i)
        {
            const uint32_t victimIdx = (start + i) % n;
            const auto victim = workers[victimIdx].get();

            if (victim == thisWorker) continue;
            if (!mosaic::utils::hasFlag(victim->m_sharingMode, WorkerSharingMode::allow_steal))
            {
                continue;
            }

            std::move_only_function<void()> stolen[k_stealBatchSize];
            size_t actualCount = victim->m_taskQueue.try_dequeue_bulk(stolen, k_stealBatchSize);

            if (actualCount == 0) continue;

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

    void executeTask(std::move_only_function<void()>& task) noexcept
    {
        try
        {
            task();
        }
        catch (const std::exception& e)
        {
            MOSAIC_ERROR("Worker {}: task threw std::exception: {}", m_idx, e.what());
        }
        catch (...)
        {
            MOSAIC_ERROR("Worker {}: task threw unknown exception.", m_idx);
        }

        task = nullptr;
    }
};

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

pieces::RefResult<ThreadPool, std::string> ThreadPool::initialize() noexcept
{
    auto cpuInfo = core::SystemInfo::getCPUInfo();

    int logical = static_cast<int>(cpuInfo.logicalCores);

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
        MOSAIC_ERROR("Cannot set affinity for non-existing worker with id {}.", _workerId);
        return;
    }

    auto nativeHandle = worker->m_thread.native_handle();
#ifdef MOSAIC_PLATFORM_WINDOWS
    HANDLE threadHandle = static_cast<HANDLE>(nativeHandle);
    DWORD_PTR affinityMask = 1ULL << (_cpuCoreId);

    SetThreadAffinityMask(threadHandle, affinityMask);
#elif defined(MOSAIC_PLATFORM_LINUX) || defined(MOSAIC_PLATFORM_MACOS)
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
        MOSAIC_ERROR("Cannot set sharing mode for non-existing worker with id {}.", _workerId);
        return;
    }

    static std::atomic<uint32_t> s_indirectAcceptingWorkers{0};

    bool currentlyAcceptsIndirect =
        mosaic::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::accept_indirect);
    bool willAcceptIndirect =
        mosaic::utils::hasFlag(_sharingMode, WorkerSharingMode::accept_indirect);

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
        MOSAIC_WARN(
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
        MOSAIC_ERROR("ThreadWorker with id {} does not exist.", _idx);
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

    MOSAIC_ERROR("ThreadWorker with debug name '{}' does not exist.", _debugName);

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

bool ThreadPool::assignTaskToWorker(std::move_only_function<void()> _task) noexcept
{
    if (m_impl->stop.load(std::memory_order_acquire))
    {
        MOSAIC_WARN("ThreadPool is shutting down, cannot assign new tasks.");
        return false;
    }

    for (uint32_t i = 0; i < m_impl->workersCount; ++i)
    {
        ThreadWorker* worker = getRandomWorker();

        if (mosaic::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::accept_indirect))
        {
            worker->m_taskQueue.enqueue(std::move(_task));
            worker->notify();
            return true;
        }
    }

    MOSAIC_INFO("No thread worker available that accepts indirect task submissions.");

    m_impl->globalTaskQueue.enqueue(std::move(_task));

    for (auto& worker : m_impl->workers)
    {
        if (mosaic::utils::hasFlag(worker->m_sharingMode, WorkerSharingMode::global_consumer))
        {
            worker->notify();
        }
    }

    return true;
}

bool ThreadPool::assignTaskToWorkerById(uint32_t _idx,
                                        std::move_only_function<void()> _task) noexcept
{
    ThreadWorker* worker = getWorkerByIdx(_idx);

    const WorkerSharingMode mode = worker->m_sharingMode;

    if (!mosaic::utils::hasFlag(mode, WorkerSharingMode::accept_direct))
    {
        MOSAIC_ERROR("Worker with id {} does not accept direct task submissions.", _idx);
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
                                               std::move_only_function<void()> _task) noexcept
{
    ThreadWorker* worker = getWorkerByDebugName(_debugName);

    const WorkerSharingMode mode = worker->m_sharingMode;

    if (!mosaic::utils::hasFlag(mode, WorkerSharingMode::accept_direct))
    {
        MOSAIC_ERROR("Worker with debug name '{}' does not accept direct task submissions.",
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
} // namespace mosaic
