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

#include "mosaic/defines.hpp"

namespace mosaic
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
    error,
    consumed // Value has been retrieved
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
    future_already_consumed
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

    alignas(MOSAIC_CACHE_LINE_SIZE) mutable std::mutex m_mutex;
    alignas(MOSAIC_CACHE_LINE_SIZE) std::condition_variable m_cv;
    std::atomic<FutureStatus> m_status;
    std::variant<std::monostate, StorageType, std::exception_ptr> m_storage;
    std::atomic<bool> m_future_retrieved;

   public:
    SharedState() : m_status(FutureStatus::pending), m_future_retrieved(false) {};

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

        FutureStatus expected = FutureStatus::pending;
        if (m_status.load(std::memory_order_relaxed) != expected)
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

        FutureStatus expected = FutureStatus::pending;
        if (m_status.load(std::memory_order_relaxed) != expected)
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

        FutureStatus expected = FutureStatus::pending;
        if (m_status.load(std::memory_order_relaxed) != expected)
        {
            throw FutureException(FutureErrorCode::promise_already_satisfied);
        }

        m_storage.template emplace<2>(std::move(_ex));
        m_status.store(FutureStatus::error, std::memory_order_release);

        lock.unlock();
        m_cv.notify_all();
    }

    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, U> get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock,
                  [this]
                  {
                      auto status = m_status.load(std::memory_order_acquire);
                      return status != FutureStatus::pending;
                  });

        auto status = m_status.load(std::memory_order_acquire);

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
                      return status != FutureStatus::pending;
                  });

        auto status = m_status.load(std::memory_order_acquire);

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
        if (status != FutureStatus::pending) return;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending) return;

            std::this_thread::yield();
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]
                  { return m_status.load(std::memory_order_acquire) != FutureStatus::pending; });
    }

    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& timeout)
    {
        auto start = std::chrono::steady_clock::now();

        auto status = m_status.load(std::memory_order_acquire);
        if (status != FutureStatus::pending) return true;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending) return true;

            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) return false;

            std::this_thread::yield();
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = timeout - elapsed;
        if (remaining <= std::chrono::duration<Rep, Period>::zero()) return false;

        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(
            lock, remaining,
            [this] { return m_status.load(std::memory_order_acquire) != FutureStatus::pending; });
    }

    template <typename Clock, typename Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& deadline)
    {
        auto status = m_status.load(std::memory_order_acquire);

        if (status != FutureStatus::pending) return true;

        for (int i = 0; i < k_spinCount; ++i)
        {
            status = m_status.load(std::memory_order_acquire);
            if (status != FutureStatus::pending) return true;

            if (Clock::now() >= deadline) return false;

            std::this_thread::yield();
        }

        if (Clock::now() >= deadline) return false;

        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_until(
            lock, deadline,
            [this] { return m_status.load(std::memory_order_acquire) != FutureStatus::pending; });
    }

    bool isReady() const noexcept
    {
        auto status = m_status.load(std::memory_order_acquire);
        return status == FutureStatus::ready || status == FutureStatus::error;
    }

    FutureStatus getStatus() const noexcept { return m_status.load(std::memory_order_acquire); }
};

/**
 * @brief Custom Future implementation optimized for task-based parallelism
 *
 * Reduces heap allocations compared to std::future and provides better control
 * over synchronization primitives.
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
    bool valid() const noexcept { return m_state != nullptr; }

    /**
     * @brief Get the result (blocks until ready)
     */
    T get()
    {
        if (!valid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->get();
    }

    /**
     * @brief Wait for the result
     */
    void wait() const
    {
        if (!valid()) throw FutureException(FutureErrorCode::no_state);

        m_state->wait();
    }

    /**
     * @brief Wait for the result with a timeout
     */
    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& _timeout) const
    {
        if (!valid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->waitFor(_timeout);
    }

    /**
     * @brief Wait for the result until a specific time point
     */
    template <typename Clock, typename Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& _deadline) const
    {
        if (!valid()) throw FutureException(FutureErrorCode::no_state);

        return m_state->waitUntil(_deadline);
    }

    /**
     * @brief Check if the result is ready (non-blocking)
     */
    bool isReady() const noexcept { return m_state && m_state->isReady(); }

    /**
     * @brief Get the current status
     */
    FutureStatus getStatus() const noexcept
    {
        return valid() ? m_state->getStatus() : FutureStatus::pending;
    }
};

/**
 * @brief Custom Promise implementation optimized for task-based parallelism
 */
template <typename T>
class TaskPromise
{
   private:
    std::shared_ptr<SharedState<T>> m_state;

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
     * @brief Set the value (for non-void types)
     */
    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, void> setValue(U&& _value)
    {
        if (!m_state) throw FutureException(FutureErrorCode::no_state);

        m_state->setValue(std::forward<U>(_value));
    }

    /**
     * @brief Set the value (for void type)
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
    bool valid() const noexcept { return m_state != nullptr; }
};

/**
 * @brief Helper to create a task with promise/future pair
 */
template <typename F, typename... Args>
inline auto makeTaskPair(F&& _f, Args&&... _args)
{
    using Ret = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    auto promise = std::make_shared<TaskPromise<Ret>>();
    TaskFuture<Ret> future = promise->getFuture();

    // Store arguments in a tuple to avoid capturing forwarding references
    auto args_tuple = std::make_tuple(std::forward<Args>(_args)...);

    std::move_only_function<void()> wrapper =
        [promise, f = std::forward<F>(_f), args_tuple = std::move(args_tuple)]() mutable
    {
        try
        {
            if constexpr (std::is_void_v<Ret>)
            {
                std::apply(std::move(f), std::move(args_tuple));
                promise->setValue();
            }
            else
            {
                promise->setValue(std::apply(std::move(f), std::move(args_tuple)));
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
} // namespace mosaic
