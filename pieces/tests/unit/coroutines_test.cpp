#include <gtest/gtest.h>
#include <variant>
#include <functional>
#include <string>

#include <pieces/utils/coroutines.hpp>

using namespace pieces;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers and Test Coroutines
////////////////////////////////////////////////////////////////////////////////////////////////////

Task<int> returnFive() { co_return 5; }

Task<int> throwError()
{
    throw std::runtime_error("oops");
    co_return 0; // unreachable
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(LambdaAwaitableTest, ReadySuspendResume)
{
    bool readyCalled = false;
    bool suspendCalled = false;
    bool resumeCalled = false;
    auto awaitable = makeAwaitable(
        [&]()
        {
            readyCalled = true;
            return true;
        },
        [&](std::coroutine_handle<>) { suspendCalled = true; },
        [&]()
        {
            resumeCalled = true;
            return 42;
        });

    // await_ready should call the ready lambda and return its value
    EXPECT_TRUE(awaitable.await_ready());
    EXPECT_TRUE(readyCalled);

    // await_suspend should call the suspend lambda
    std::coroutine_handle<> dummy;
    awaitable.await_suspend(dummy);
    EXPECT_TRUE(suspendCalled);

    // await_resume should call the resume lambda and return its result
    int value = awaitable.await_resume();
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(resumeCalled);
}

TEST(LambdaAwaitableTest, ResumeThrows)
{
    auto awaitable = makeAwaitable([]() { return true; }, [](std::coroutine_handle<>) {},
                                   []() -> int { throw std::logic_error("fail"); });

    EXPECT_TRUE(awaitable.await_ready());
    EXPECT_THROW(awaitable.await_resume(), std::logic_error);
}

TEST(PromiseTypeInt, InitialFinalSuspendNoexcept)
{
    using P = Task<int>::promise_type;
    static_assert(noexcept(std::declval<P>().initial_suspend()), "initial_suspend not noexcept");
    static_assert(noexcept(std::declval<P>().final_suspend()), "final_suspend not noexcept");

    auto init = P{}.initial_suspend();
    EXPECT_FALSE(init.await_ready());

    auto fin = P{}.final_suspend();
    EXPECT_FALSE(fin.await_ready());
}

// Test that a simple coroutine returns the correct value when run to completion. We use a manual
// runner coroutine for testing
struct ManualRunner
{
    struct promise_type
    {
        std::optional<int> result;

        std::exception_ptr ex;

        std::coroutine_handle<> continuation;

        ManualRunner get_return_object() noexcept
        {
            return ManualRunner{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct FinalAwaiter
        {
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<promise_type> h) noexcept
            {
                if (h.promise().continuation)
                {
                    h.promise().continuation.resume();
                }
            }

            void await_resume() noexcept {}
        };

        auto final_suspend() noexcept { return FinalAwaiter{}; }

        void unhandled_exception() noexcept { ex = std::current_exception(); }

        void return_value(int v) noexcept { result = v; }
    };

    using HandleType = std::coroutine_handle<promise_type>;

    HandleType coro;

    explicit ManualRunner(HandleType h) : coro(h) {}

    ~ManualRunner()
    {
        if (coro) coro.destroy();
    }

    // Static run helper that drives a Task<int> to completion
    static int run(pieces::Task<int> task)
    {
        // Inner coroutine that awaits the task and returns its value

        auto inner = [&](pieces::Task<int> t) -> ManualRunner
        {
            int v = co_await t;

            co_return v;
        }(std::move(task));

        // Ensure final suspend will resume to a no-op

        inner.coro.promise().continuation = std::noop_coroutine();

        // Drive inner coroutine to completion

        inner.coro.resume();

        if (inner.coro.promise().ex) std::rethrow_exception(inner.coro.promise().ex);

        return *inner.coro.promise().result;
    }
};

TEST(TaskTest, ReturnValue)
{
    Task<int> t = returnFive();
    ManualRunner runner = [&]() -> ManualRunner { co_return 0; }();
    int result = runner.run(returnFive());

    EXPECT_EQ(result, 5);
}

TEST(TaskTest, ExceptionPropagates)
{
    Task<int> t = throwError();
    ManualRunner runner = [&]() -> ManualRunner { co_return 0; }();

    EXPECT_THROW(runner.run(throwError()), std::runtime_error);
}
