#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace pieces
{

/**
 * @brief A simple coroutine task class that can be used to create coroutines in C++20 and later.
 *
 * The class implements the promise type and the coroutine handle for the coroutine according to the
 * compiler and C++ standard library requirements.
 *
 * @tparam T The type of the value returned by the coroutine. If T is void, the coroutine does not
 * return a value.
 */
template <typename T>
class Task
{
   public:
    struct promise_type
    {
        std::optional<T> m_value;
        std::exception_ptr m_exception;
        std::coroutine_handle<> m_continuation;

        Task get_return_object() noexcept
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter
        {
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<promise_type> _handle) noexcept
            {
                if (_handle.promise().m_continuation)
                {
                    _handle.promise().m_continuation.resume();
                }
            }

            void await_resume() noexcept {}
        };

        auto final_suspend() noexcept { return final_awaiter{}; }

        void unhandled_exception() noexcept { m_exception = std::current_exception(); }

        template <typename U>
        void return_value(U&& _value) noexcept
        {
            m_value = std::forward<U>(_value);
        }
    };

    using HandleType = std::coroutine_handle<promise_type>;

    explicit Task(HandleType _handle) noexcept : m_coro(_handle) {}

    ~Task()
    {
        if (m_coro) m_coro.destroy();
    }

    Task(Task&& _other) noexcept : m_coro(_other.m_coro) { _other.m_coro = nullptr; }

    Task& operator=(Task&& _other) noexcept
    {
        if (this != &_other)
        {
            if (m_coro) m_coro.destroy();
            m_coro = _other.m_coro;
            _other.m_coro = nullptr;
        }

        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    auto operator co_await() & noexcept
    {
        struct Awaiter
        {
            HandleType m_coro;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<> _caller) noexcept
            {
                m_coro.promise().m_continuation = _caller;
                m_coro.resume();
            }

            T await_resume()
            {
                if (m_coro.promise().m_exception)
                    std::rethrow_exception(m_coro.promise().m_exception);
                return std::move(*m_coro.promise().m_value);
            }
        };

        return Awaiter{m_coro};
    }

   private:
    HandleType m_coro;
};

/**
 * @brief A specialization of the Task class for void return type.
 *
 * NOTE: This specialization is needed because you can't store or move a void type. So we use a
 * special case for void return type.
 *
 * @see Task<T> for more information.
 */
template <>
class Task<void>
{
   public:
    struct promise_type
    {
        std::exception_ptr m_exception;
        std::coroutine_handle<> m_continuation;

        Task get_return_object() noexcept
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter
        {
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<promise_type> _handle) noexcept
            {
                if (_handle.promise().m_continuation) _handle.promise().m_continuation.resume();
            }

            void await_resume() noexcept {}
        };

        auto final_suspend() noexcept { return final_awaiter{}; }

        void unhandled_exception() noexcept { m_exception = std::current_exception(); }

        void return_void() noexcept {}
    };

    using HandleType = std::coroutine_handle<promise_type>;

    explicit Task(HandleType _handle) noexcept : m_coro(_handle) {}

    ~Task()
    {
        if (m_coro) m_coro.destroy();
    }

    Task(Task&& _other) noexcept : m_coro(_other.m_coro) { _other.m_coro = nullptr; }

    Task& operator=(Task&& _other) noexcept
    {
        if (this != &_other)
        {
            if (m_coro) m_coro.destroy();
            m_coro = _other.m_coro;
            _other.m_coro = nullptr;
        }

        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    auto operator co_await() & noexcept
    {
        struct Awaiter
        {
            HandleType m_coro;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<> _caller) noexcept
            {
                m_coro.promise().m_continuation = _caller;
                m_coro.resume();
            }

            void await_resume()
            {
                if (m_coro.promise().m_exception)
                    std::rethrow_exception(m_coro.promise().m_exception);
            }
        };

        return Awaiter{m_coro};
    }

   private:
    HandleType m_coro;
};

/**
 * @brief A simple awaitable class that can be used to create coroutines in C++20 and later.
 *
 * @tparam Ready The type of the ready function. The function should return a boolean value.
 * @tparam Suspend The type of the suspend function. The function should take a coroutine handle as
 * a parameter.
 * @tparam Resume The type of the resume function. The function should return the value of the
 * coroutine.
 */
template <typename Ready, typename Suspend, typename Resume>
struct LambdaAwaitable
{
    Ready m_ready;
    Suspend m_suspend;
    Resume m_resume;

    constexpr LambdaAwaitable(Ready&& _ready, Suspend&& _suspend, Resume&& _resume)
        : m_ready(std::move(_ready)),
          m_suspend(std::move(_suspend)),
          m_resume(std::move(_resume)) {};

    constexpr bool await_ready() noexcept(noexcept(std::declval<Ready&>()())) { return m_ready(); }

    constexpr void await_suspend(std::coroutine_handle<> _handle) noexcept(
        noexcept(std::declval<Suspend&>()(_handle)))
    {
        m_suspend(_handle);
    }

    constexpr auto await_resume() noexcept(noexcept(std::declval<Resume&>()()))
        -> decltype(m_resume())
    {
        return m_resume();
    }
};

/**
 * @brief A helper function to create a LambdaAwaitable object.
 *
 * @tparam Ready The type of the ready function. The function should return a boolean value.
 * @tparam Suspend The type of the suspend function. The function should take a coroutine handle as
 * a parameter.
 * @tparam Resume The type of the resume function. The function should return the value of the
 * coroutine.
 * @param ready The ready function.
 * @param suspend The suspend function.
 * @param resume The resume function.
 * @return A LambdaAwaitable object.
 */
template <typename Ready, typename Suspend, typename Resume>
constexpr auto makeAwaitable(Ready&& _ready, Suspend&& _suspend, Resume&& _resume)
{
    using Awaitable =
        LambdaAwaitable<std::decay_t<Ready>, std::decay_t<Suspend>, std::decay_t<Resume>>;

    return Awaitable{std::forward<Ready>(_ready), std::forward<Suspend>(_suspend),
                     std::forward<Resume>(_resume)};
}

} // namespace pieces
