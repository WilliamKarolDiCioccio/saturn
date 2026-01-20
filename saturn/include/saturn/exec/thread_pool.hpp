#pragma once

#include "saturn/defines.hpp"

#include <pieces/core/result.hpp>
#include <pieces/utils/enum_flags.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/core/sys_info.hpp"

#include "task_future.hpp"
#include "move_only_task.hpp"

namespace saturn
{
namespace exec
{

/**
 * @brief Describes how a worker shares or isolates its workload.
 *
 * Each flag enables or restricts specific scheduling behaviors.
 * These can be combined to form more complex policies.
 */
enum class WorkerSharingMode : uint8_t
{
    none = 0,
    allow_steal = 1 << 1,     /// Other workers can steal tasks from it.
    accept_direct = 1 << 3,   /// The thread pool can assign tasks directly to this worker.
    accept_indirect = 1 << 2, /// The thread pool can assign tasks through automatic dispatch.
    global_consumer = 1 << 6, /// Can pull tasks directly from the global queue.
};

SATURN_DEFINE_ENUM_FLAGS_OPERATORS(WorkerSharingMode)

/// Common presets for worker behavior.
namespace worker_sharing_presets
{

inline constexpr WorkerSharingMode exclusive = WorkerSharingMode::accept_direct;

inline constexpr WorkerSharingMode shared =
    WorkerSharingMode::allow_steal | WorkerSharingMode::accept_direct |
    WorkerSharingMode::accept_indirect | WorkerSharingMode::global_consumer;

inline constexpr WorkerSharingMode shared_no_steal = WorkerSharingMode::accept_direct |
                                                     WorkerSharingMode::accept_indirect |
                                                     WorkerSharingMode::global_consumer;

} // namespace worker_sharing_presets

/**
 * @brief Statistics snapshot for a single worker (copy, no atomics exposed).
 *
 * All counters are cumulative since pool initialization or last resetStats() call.
 */
struct WorkerStatsSnapshot
{
    uint64_t tasksExecuted;           /// Total tasks executed by this worker.
    uint64_t tasksStolen;             /// Total tasks stolen from other workers.
    uint64_t stealAttempts;           /// Total steal attempts made by this worker.
    uint64_t tasksReceivedFromGlobal; /// Total tasks received from global queue.
    size_t currentQueueSize;          /// Approximate current queue size.
};

/**
 * @brief Aggregated statistics snapshot for the entire pool.
 *
 * Computed lazily by iterating all workers - O(workers) cost.
 */
struct PoolStatsSnapshot
{
    uint64_t totalTasksExecuted;
    uint64_t totalTasksStolen;
    uint64_t totalStealAttempts;
    uint64_t totalTasksReceivedFromGlobal;
    size_t totalQueuedTasks;

    /// Compute average tasks stolen per steal attempt. Can exceed 1.0 due to batch stealing.
    /// Returns 0.0 if no attempts made.
    [[nodiscard]] double stealSuccessRate() const noexcept
    {
        return totalStealAttempts > 0
                   ? static_cast<double>(totalTasksStolen) / static_cast<double>(totalStealAttempts)
                   : 0.0;
    }
};

/**
 * @brief A thread worker wraps a single thread and manages its task queue.
 *
 * Workers can have different sharing modes that dictate how they interact with the task system.
 *
 * @see WorkerSharingMode
 */
class ThreadWorker;

/**
 * @brief A thread pool manages a collection of worker threads for concurrent task execution.
 *
 * It provides methods to enqueue tasks to either the global queue or specific workers. It creates a
 * worker for each logical CPU core. It allows setting CPU affinity for workers to optimize
 * performance by reducing migration between cores and improving cache locality.
 *
 * @see ThreadWorker
 */
class SATURN_API ThreadPool
{
   private:
    static ThreadPool* g_instance;

    struct Impl;

    Impl* m_impl = nullptr;

    friend class ThreadWorker;

   public:
    ThreadPool();
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

   public:
    pieces::RefResult<ThreadPool, std::string> initialize(
        const saturn::core::CPUInfo& _cpuInfo) noexcept;
    void shutdown() noexcept;

    /**
     * @brief Enqueues a task to the global task queue.
     *
     * @tparam F The type of the callable to execute.
     * @tparam Args The types of the arguments to forward to the task.
     * @param f The callable to execute.
     * @param args The arguments to forward to the task.
     * @return std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>,
     * std::decay_t<Args>...>>>
     *
     * @see assignTaskToGlobal for details on task assignment behavior.
     */
    template <typename F, typename... Args>
    std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>
    enqueueToGlobal(F&& f, Args&&... args)
    {
        using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto [wrapper, future] = makeTaskPair(std::forward<F>(f), std::forward<Args>(args)...);

        if (!assignTaskToGlobal(std::move(wrapper))) return std::nullopt;

        return std::make_optional(std::move(future));
    }

    /**
     * @brief Tries to perform optimal assignment of a task to worker threads and fallbacks to the
     * global queue if no suitable worker is found.
     *
     * @tparam F The type of the callable to execute.
     * @tparam Args The types of the arguments to forward to the task.
     * @param _f The callable to execute.
     * @param _args The arguments to forward to the task.
     * @return std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>,
     * std::decay_t<Args>...>>>
     *
     * @see assignTaskToWorker for details on task assignment behavior.
     */
    template <typename F, typename... Args>
    std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>
    enqueueToWorker(F&& _f, Args&&... _args)
    {
        using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto [wrapper, future] = makeTaskPair(std::forward<F>(_f), std::forward<Args>(_args)...);

        if (!assignTaskToWorker(std::move(wrapper))) return std::nullopt;

        return std::make_optional(std::move(future));
    }

    /**
     * @brief Enqueues a task to a worker thread by its ID.
     *
     * @tparam F The type of the callable to execute.
     * @tparam Args The types of the arguments to forward to the task.
     * @param _id The ID of the worker thread.
     * @param _f The callable to execute.
     * @param _args The arguments to forward to the task.
     * @return std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>,
     * std::decay_t<Args>...>>>
     *
     * @see assignTaskToWorkerById for details on task assignment behavior.
     */
    template <typename F, typename... Args>
    std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>
    enqueueToWorkerById(uint32_t _id, F&& _f, Args&&... _args)
    {
        using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto [wrapper, future] = makeTaskPair(std::forward<F>(_f), std::forward<Args>(_args)...);

        if (!assignTaskToWorkerById(_id, std::move(wrapper))) return std::nullopt;

        return std::make_optional(std::move(future));
    }

    /**
     * @brief Enqueues a task to a worker thread by its debug name.
     *
     * @tparam F The type of the callable to execute.
     * @tparam Args The types of the arguments to forward to the task.
     * @param _debugName The debug name of the worker thread.
     * @param _f The callable to execute.
     * @param _args The arguments to forward to the task.
     * @return std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>,
     * std::decay_t<Args>...>>>
     *
     * @see assignTaskToWorkerByDebugName for details on task assignment behavior.
     */
    template <typename F, typename... Args>
    std::optional<TaskFuture<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>>
    enqueueToWorkerByDebugName(const std::string& _debugName, F&& _f, Args&&... _args)
    {
        using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto [wrapper, future] = makeTaskPair(std::forward<F>(_f), std::forward<Args>(_args)...);

        if (!assignTaskToWorkerByDebugName(_debugName, std::move(wrapper))) return std::nullopt;

        return std::make_optional(std::move(future));
    }

    void setWorkerAffinity(uint32_t _workerId, size_t _cpuCoreId) noexcept;
    void setWorkerSharingMode(uint32_t _workerId, WorkerSharingMode _sharingMode) noexcept;

    [[nodiscard]] bool isRunning() const noexcept;

    [[nodiscard]] uint32_t getWorkersCount() const noexcept;
    [[nodiscard]] uint32_t getBusyWorkersCount() const noexcept;
    [[nodiscard]] uint32_t getIdleWorkersCount() const noexcept;

    /// @brief Get statistics snapshot for a specific worker.
    [[nodiscard]] WorkerStatsSnapshot getWorkerStats(uint32_t _workerIdx) const noexcept;

    /// @brief Get aggregated statistics for entire pool (lazy - iterates all workers).
    [[nodiscard]] PoolStatsSnapshot getPoolStats() const noexcept;

    /// @brief Reset all statistics counters across all workers.
    void resetStats() noexcept;

    /// @brief Get a random worker from the pool using a uniform distribution.
    [[nodiscard]] ThreadWorker* getRandomWorker() const noexcept;

    /// @brief Get a worker by its unique ID.
    [[nodiscard]] ThreadWorker* getWorkerByIdx(uint32_t _idx) const noexcept;

    /// @brief Get a worker by its debug name.
    [[nodiscard]] ThreadWorker* getWorkerByDebugName(const std::string& _debugName) const noexcept;

    [[nodiscard]] static inline ThreadPool* getInstance()
    {
        if (!g_instance) SATURN_ERROR("ThreadPool has not been created yet!");

        return g_instance;
    }

   private:
    void setupWorker(uint32_t _idx);
    void startupWorker(uint32_t _idx);

    /**
     * @brief Assigns a task to the global task queue.
     *
     * @return true if the task was successfully enqueued to the global queue.
     * @return false if the thread pool is shutting down and cannot accept new tasks.
     */
    bool assignTaskToGlobal(MoveOnlyTask<void()> _task) noexcept;

    /**
     * @brief Tries to perform optimal assignment of a task to a worker.
     *
     * @return true if the task was successfully indirectly assigned to a worker or enqueued to the
     * global queue.
     * @return false if the thread pool is shutting down and cannot accept new tasks.
     */
    bool assignTaskToWorker(MoveOnlyTask<void()> _task) noexcept;

    /**
     * @brief Assigns a task directly to a specific worker by its ID.

     * @return true if the task was successfully directly assigned to the worker.
     * @return false if the thread pool is shutting down or the worker does not allow direct task
     * assignments.
     */
    bool assignTaskToWorkerById(uint32_t _idx, MoveOnlyTask<void()> _task) noexcept;

    /**
     * @brief Assigns a task directly to a specific worker by its debug name.
     *
     * @return true if the task was successfully directly assigned to the worker.
     * @return false if the thread pool is shutting down or the worker does not allow direct task
     * assignments.
     */
    bool assignTaskToWorkerByDebugName(const std::string& _debugName,
                                       MoveOnlyTask<void()> _task) noexcept;
};

} // namespace exec
} // namespace saturn
