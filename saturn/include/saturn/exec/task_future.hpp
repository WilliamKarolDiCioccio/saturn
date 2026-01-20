#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <tuple>
#include <chrono>

#include "saturn/defines.hpp"
#include "saturn/exec/move_only_task.hpp"

namespace saturn
{
namespace exec
{

/**
 * @brief Status of a task future
 */
enum class FutureStatus
{
    pending,
    ready,
    executing,
    error,
    cancelled,
    consumed,
};

/**
 * @brief Error codes for future/promise operations
 */
enum class FutureErrorCode
{
    no_state,
    promise_already_satisfied,
    future_already_retrieved,
    broken_promise,
    future_already_consumed,
};

/**
 * @brief Exception thrown by future/promise operations
 */
class FutureException : public std::logic_error
{
   private:
    FutureErrorCode m_code;

   public:
    explicit FutureException(FutureErrorCode _code)
        : std::logic_error(getMessage(_code)), m_code(_code) {};

    FutureErrorCode code() const noexcept { return m_code; }

   private:
    static const char* getMessage(FutureErrorCode _code) noexcept
    {
        switch (_code)
        {
            case FutureErrorCode::no_state:
                return "No associated state";
            case FutureErrorCode::promise_already_satisfied:
                return "Promise already satisfied";
            case FutureErrorCode::future_already_retrieved:
                return "Future already retrieved";
            case FutureErrorCode::broken_promise:
                return "Broken promise";
            case FutureErrorCode::future_already_consumed:
                return "Future value already consumed";
            default:
                return "Unknown future error";
        }
    }
};

/**
 * @brief Shared state between Promise and Future
 *
 * Uses a single allocation for the entire state, including the result storage.
 * Optimized for cache locality and minimal synchronization overhead.
 */
template <typename T>
class SharedState
{
   public:
    using ValueType = T;
    using StorageType = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

   private:
    static constexpr int k_spinCount = 100;

    alignas(SATURN_CACHE_LINE_SIZE) mutable std::mutex m_mutex;
    alignas(SATURN_CACHE_LINE_SIZE) std::condition_variable m_cv;
    std::atomic<FutureStatus> m_status;
    std::variant<std::monostate, StorageType, std::exception_ptr> m_storage;
    std::atomic<bool> m_future_retrieved;
    std::atomic<bool> m_cancellationRequested;
    std::atomic<bool> m_cancelled;

   public:
    SharedState()
        : m_status(FutureStatus::pending),
          m_future_retrieved(false),
          m_cancellationRequested(false),
          m_cancelled(false) {};

    ~SharedState() = default;

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;

   public:
    bool tryRetrieveFuture()
    {
        bool expected = false;
        return m_future_retrieved.compare_exchange_strong(expected, true,
                                                          std::memory_order_acq_rel);
    }

    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, void> setValue(U&& _value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto current = m_status.load(std::memory_order_relaxed);
        if (current != FutureStatus::pending && current != FutureStatus::executing)
        {
            throw FutureException(FutureErrorCode::promise_already_satisfied);
        }

        m_storage.template emplace<1>(std::forward<U>(_value));
        m_status.store(FutureStatus::ready, std::memory_order_release);

        lock.unlock();
        m_cv.notify_all();
    }

    template <typename U = T>
    std::enable_if_t<std::is_void_v<U>, void> setValue()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto current = m_status.load(std::memory_order_relaxed);
        if (current != FutureStatus::pending && current != FutureStatus::executing)
        {
            throw FutureException(FutureErrorCode::promise_already_satisfied);
        }

        m_storage.template emplace<1>(std::monostate{});
        m_status.store(FutureStatus::ready, std::memory_order_release);

        lock.unlock();
        m_cv.notify_all();
    }

    void setException(std::exception_ptr _ex)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        auto current = m_status.load(std::memory_order_relaxed);
        if (current != FutureStatus::pending && current != FutureStatus::executing)
        {
            throw FutureException(FutureErrorCode::promise_already_satisfied);
        }

        m_storage.template emplace<2>(std::move(_ex));
        m_status.store(FutureStatus::error, std::memory_order_release);

        lock.unlock();
        m_cv.notify_all();
    }

    bool isCancellationRequested() const noexcept
    {
        return m_cancellationRequested.load(std::memory_order_acquire);
    }

    bool isCancelled() const noexcept { return m_cancelled.load(std::memory_order_acquire); }

    bool isExecuting() const noexcept
    {
        return m_status.load(std::memory_order_acquire) == FutureStatus::executing;
    }

    bool isReady() const noexcept
    {
        auto status = m_status.load(std::memory_order_acquire);
        return status == FutureStatus::ready || status == FutureStatus::error;
    }

    bool tryMarkExecuting() noexcept
    {
        FutureStatus expected = FutureStatus::pending;
        return m_status.compare_exchange_strong(expected, FutureStatus::executing,
                                                std::memory_order_acq_rel);
    }

    void markReady() noexcept
    {
        FutureStatus expected = FutureStatus::executing;
        if (m_status.load(std::memory_order_acquire) == expected &&
            !m_cancellationRequested.load(std::memory_order_acquire))
        {
            m_status.compare_exchange_strong(expected, FutureStatus::ready,
                                             std::memory_order_acq_rel);
        }
    }

    bool cancel() noexcept
    {
        FutureStatus current = m_status.load(std::memory_order_acquire);

        if (current == FutureStatus::pending)
        {
            if (m_status.compare_exchange_strong(current, FutureStatus::cancelled,
                                                 std::memory_order_acq_rel))
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_cancelled.store(true, std::memory_order_release);

                lock.unlock();
                m_cv.notify_all();

                return true;
            }
        }

        if (current == FutureStatus::executing)
        {
            m_cancellationRequested.store(true, std::memory_order_release);
            return true;
        }

        return false;
    }

    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, U> get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock,
                  [this]
                  {
                      auto status = m_status.load(std::memory_order_acquire);
                      return status != FutureStatus::pending && status != FutureStatus::executing;
                  });

        auto status = m_status.load(std::memory_order_acquire);

        if (status == FutureStatus::cancelled)
        {
            throw FutureException(FutureErrorCode::broken_promise);
        }

        if (status == FutureStatus::consumed)
        {
            throw FutureException(FutureErrorCode::future_already_consumed);
        }

        if (status == FutureStatus::error)
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }

        auto result = std::move(std::get<StorageType>(m_storage));
        m_status.store(FutureStatus::consumed, std::memory_order_release);
        return result;
    }

    template <typename U = T>
    std::enable_if_t<std::is_void_v<U>, void> get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock,
                  [this]
                  {
                      auto status = m_status.load(std::memory_order_acquire);
                      return status != FutureStatus::pending && status != FutureStatus::executing;
                  });

        auto status = m_status.load(std::memory_order_acquire);

        if (status == FutureStatus::cancelled)
        {
            throw FutureException(FutureErrorCode::broken_promise);
        }

        if (status == FutureStatus::consumed)
        {
            throw FutureException(FutureErrorCode::future_already_consumed);
        }

        if (status == FutureStatus::error)
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        }

        m_status.store(FutureStatus::consumed, std::memory_order_release);
    }

    void wait()
    {
        auto status = m_status.load(std::memory_order_acquire);
        if (status != FutureStatus::pending && status != FutureStatus::executing) return;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending && status != FutureStatus::executing) return;

            std::this_thread::yield();
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock,
                  [this]
                  {
                      auto status = m_status.load(std::memory_order_acquire);
                      return status != FutureStatus::pending && status != FutureStatus::executing;
                  });
    }

    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& timeout)
    {
        auto start = std::chrono::steady_clock::now();

        auto status = m_status.load(std::memory_order_acquire);
        if (status != FutureStatus::pending && status != FutureStatus::executing) return true;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending && status != FutureStatus::executing) return true;

            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) return false;

            std::this_thread::yield();
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = timeout - elapsed;
        if (remaining <= std::chrono::duration<Rep, Period>::zero()) return false;

        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, remaining,
                             [this]
                             {
                                 auto status = m_status.load(std::memory_order_acquire);
                                 return status != FutureStatus::pending &&
                                        status != FutureStatus::executing;
                             });
    }

    template <typename Clock, typename Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& _deadline)
    {
        auto status = m_status.load(std::memory_order_acquire);
        if (status != FutureStatus::pending && status != FutureStatus::executing) return true;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending && status != FutureStatus::executing) return true;

            if (Clock::now() >= _deadline) return false;

            std::this_thread::yield();
        }

        if (Clock::now() >= _deadline) return false;

        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_until(lock, _deadline,
                               [this]
                               {
                                   auto status = m_status.load(std::memory_order_acquire);
                                   return status != FutureStatus::pending &&
                                          status != FutureStatus::executing;
                               });
    }

    FutureStatus getStatus() const noexcept { return m_status.load(std::memory_order_acquire); }
};

template <typename T>
class ExecutionToken;

/**
 * @brief Custom Future implementation
 *
 * Reduces heap allocations compared to std::future and provides cancellation support.
 */
template <typename T>
class TaskFuture
{
   private:
    std::shared_ptr<SharedState<T>> m_state;

   public:
    TaskFuture() = default;

    explicit TaskFuture(std::shared_ptr<SharedState<T>> _state) : m_state(std::move(_state)) {}

    TaskFuture(const TaskFuture&) = delete;
    TaskFuture& operator=(const TaskFuture&) = delete;

    TaskFuture(TaskFuture&&) noexcept = default;
    TaskFuture& operator=(TaskFuture&&) noexcept = default;

   public:
    /**
     * @brief Check if the future has a shared state
     */
    bool isValid() const noexcept { return m_state != nullptr; }

    /**
     * @brief Check if the task was cancelled
     */
    bool isCancelled() const noexcept { return m_state && m_state->isCancelled(); }

    /**
     * @brief Check if the task is currently executing
     */
    bool isExecuting() const noexcept { return m_state && m_state->isExecuting(); }

    /**
     * @brief Check if the result is ready (non-blocking)
     */
    bool isReady() const noexcept { return m_state && m_state->isReady(); }

    /**
     * @brief Attempt to cancel the task
     *
     * @return true if successfully cancelled, false if already completed
     */
    bool cancel() noexcept { return m_state && m_state->cancel(); }

    /**
     * @brief Try to mark the task as executing
     *
     * @return true if successfully marked as executing, false if already completed
     */
    bool tryMarkExecuting() noexcept { return m_state && m_state->tryMarkExecuting(); }

    /**
     * @brief Get the result
     *
     * @note This call will block until the result is available
     */
    T get()
    {
        if (!isValid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->get();
    }

    /**
     * @brief Wait for the result
     */
    void wait() const
    {
        if (!isValid()) throw FutureException(FutureErrorCode::no_state);

        m_state->wait();
    }

    /**
     * @brief Wait for the result with a timeout
     *
     * @param _timeout The maximum duration to wait
     * @return true if the result is ready within the timeout, false otherwise
     */
    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& _timeout) const
    {
        if (!isValid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->waitFor(_timeout);
    }

    /**
     * @brief Wait for the result until a specific time point
     *
     * @param _deadline The time point to wait until
     * @return true if the result is ready before the deadline, false otherwise
     */
    template <typename Clock, typename Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& _deadline) const
    {
        if (!isValid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->waitUntil(_deadline);
    }

    /**
     * @brief Get the current status
     */
    FutureStatus getStatus() const noexcept
    {
        return isValid() ? m_state->getStatus() : FutureStatus::pending;
    }
};

/**
 * @brief Custom Promise implementation
 */
template <typename T>
class TaskPromise
{
   private:
    std::shared_ptr<SharedState<T>> m_state;

    friend class ExecutionToken<T>;

   public:
    TaskPromise() : m_state(std::make_shared<SharedState<T>>()) {}

    TaskPromise(const TaskPromise&) = delete;
    TaskPromise& operator=(const TaskPromise&) = delete;

    TaskPromise(TaskPromise&&) noexcept = default;
    TaskPromise& operator=(TaskPromise&&) noexcept = default;

    ~TaskPromise()
    {
        if (m_state && m_state->getStatus() == FutureStatus::pending)
        {
            try
            {
                m_state->setException(
                    std::make_exception_ptr(FutureException(FutureErrorCode::broken_promise)));
            }
            catch (...)
            {
                // We can ignore exceptions here because the promise has been satisfied
            }
        }
    }

   public:
    /**
     * @brief Get the associated future
     */
    TaskFuture<T> getFuture()
    {
        if (!m_state) throw FutureException(FutureErrorCode::no_state);

        if (!m_state->tryRetrieveFuture())
        {
            throw FutureException(FutureErrorCode::future_already_retrieved);
        }

        return TaskFuture<T>(m_state);
    }

    /**
     * @brief Set the value
     * @note This overload is only enabled for non-void types
     */
    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, void> setValue(U&& _value)
    {
        if (!m_state) throw FutureException(FutureErrorCode::no_state);

        m_state->setValue(std::forward<U>(_value));
    }

    /**
     * @brief Set the value
     * @note This overload is only enabled for void types
     */
    template <typename U = T>
    std::enable_if_t<std::is_void_v<U>, void> setValue()
    {
        if (!m_state) throw FutureException(FutureErrorCode::no_state);

        m_state->setValue();
    }

    /**
     * @brief Set an exception
     */
    void setException(std::exception_ptr _ex)
    {
        if (!m_state) throw FutureException(FutureErrorCode::no_state);

        m_state->setException(std::move(_ex));
    }

    /**
     * @brief Check if the promise has a shared state
     */
    bool isValid() const noexcept { return m_state != nullptr; }

    /**
     * @brief Get the associated shared state
     */
    std::shared_ptr<SharedState<T>> getState() const noexcept { return m_state; }
};

/**
 * @brief Token representing execution of a task
 *
 * Marks the task as executing upon creation and ready upon destruction.
 * Can be used to ensure proper state transitions in task executors.
 *
 * @tparam T The type of the task result
 */
template <typename T>
class ExecutionToken
{
   private:
    std::shared_ptr<SharedState<T>> m_state;
    bool m_acquired{false};

   public:
    ExecutionToken(const TaskPromise<T>& _promise) : m_state(_promise.getState())
    {
        if (m_state) m_acquired = m_state->tryMarkExecuting();
    }

    ~ExecutionToken() = default;

    ExecutionToken(const ExecutionToken&) = delete;
    ExecutionToken& operator=(const ExecutionToken&) = delete;

    ExecutionToken(ExecutionToken&& other) noexcept
        : m_state(std::move(other.m_state)), m_acquired(other.m_acquired)
    {
        other.m_acquired = false;
    }

    ExecutionToken& operator=(ExecutionToken&& other) noexcept
    {
        if (this == &other) return *this;

        if (m_acquired && m_state) m_state->markReady();

        m_state = std::move(other.m_state);
        m_acquired = other.m_acquired;
        other.m_acquired = false;

        return *this;
    }

    /**
     * @brief Check if execution was successfully acquired
     */
    explicit operator bool() const noexcept { return m_acquired; }

    /**
     * @brief Check if cancellation was requested during execution
     */
    bool isCancellationRequested() const noexcept
    {
        return m_state && m_state->isCancellationRequested();
    }

    /**
     * @brief Get the associated future state
     */
    std::shared_ptr<SharedState<T>> getState() const noexcept { return m_state; }
};

/**
 * @brief Helper to create a task with promise/future pair
 *
 * @tparam F Function type
 * @tparam Args Argument types
 * @param _f Function to execute
 * @param _args Arguments to pass to the function
 * @return A pair of the task wrapper and the associated future
 */
template <typename F, typename... Args>
inline auto makeTaskPair(F&& _f, Args&&... _args)
{
    using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    auto promise = std::make_shared<TaskPromise<Ret>>();
    TaskFuture<Ret> future = promise->getFuture();

    auto argsTuple = std::make_tuple(std::forward<Args>(_args)...);

    MoveOnlyTask<void()> wrapper =
        [promise, f = std::forward<F>(_f), argsTuple = std::move(argsTuple)]() mutable
    {
        ExecutionToken<Ret> token(*promise);

        if (!token) return;

        try
        {
            if constexpr (std::is_void_v<Ret>)
            {
                std::apply(std::move(f), std::move(argsTuple));

                if (!token.isCancellationRequested()) promise->setValue();
            }
            else
            {
                auto result = std::apply(std::move(f), std::move(argsTuple));

                if (!token.isCancellationRequested()) promise->setValue(std::move(result));
            }
        }
        catch (...)
        {
            promise->setException(std::current_exception());
        }
    };

    return std::make_pair(std::move(wrapper), std::move(future));
}

} // namespace exec
} // namespace saturn
