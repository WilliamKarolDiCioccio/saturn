#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "saturn/exec/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <numeric>
#include <algorithm>
#include <latch>
#include <barrier>

using namespace saturn::exec;
using namespace std::chrono_literals;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture
////////////////////////////////////////////////////////////////////////////////////////////////////

class ThreadPoolTest : public ::testing::Test
{
   protected:
    std::unique_ptr<ThreadPool> pool;

    void SetUp() override
    {
        saturn::core::CPUInfo mockCPUInfo;
        mockCPUInfo.logicalCores = 8;
        mockCPUInfo.physicalCores = 4;

        pool = std::make_unique<ThreadPool>();
        auto result = pool->initialize(mockCPUInfo);

        ASSERT_TRUE(result.isOk()) << "Failed to initialize thread pool";
    }

    void TearDown() override
    {
        pool->shutdown();
        pool.reset();
    }

    template <typename Predicate>
    bool waitFor(Predicate pred, std::chrono::milliseconds timeout = 1000ms)
    {
        auto start = std::chrono::steady_clock::now();

        while (!pred())
        {
            if (std::chrono::steady_clock::now() - start > timeout) return false;
            std::this_thread::sleep_for(1ms);
        }

        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Functionality Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, Initialization)
{
    EXPECT_TRUE(pool->isRunning());
    EXPECT_GT(pool->getWorkersCount(), 0);

    std::atomic<int> counter{0};
    std::vector<TaskFuture<void>> futures;

    for (int i = 0; i < 10; ++i)
    {
        auto f = pool->enqueueToWorker([&counter]() { counter.fetch_add(1); });
        if (f.has_value()) futures.push_back(std::move(*f));
    }

    for (auto& f : futures) f.wait();

    EXPECT_EQ(counter.load(), 10);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_GT(pool->getIdleWorkersCount(), 0);
}

TEST_F(ThreadPoolTest, SimpleTaskExecution)
{
    std::atomic<bool> executed{false};

    auto future = pool->enqueueToWorker([&executed]() { executed.store(true); });

    ASSERT_TRUE(future.has_value());
    future->wait();

    EXPECT_TRUE(executed.load());
}

TEST_F(ThreadPoolTest, TaskWithReturnValue)
{
    auto future = pool->enqueueToWorker([]() { return 42; });

    ASSERT_TRUE(future.has_value());
    EXPECT_EQ(future->get(), 42);
}

TEST_F(ThreadPoolTest, TaskWithArguments)
{
    auto add = [](int a, int b) { return a + b; };

    auto future = pool->enqueueToWorker(add, 10, 32);

    ASSERT_TRUE(future.has_value());
    EXPECT_EQ(future->get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasksExecution)
{
    constexpr int numTasks = 100;
    std::atomic<int> counter{0};
    std::vector<TaskFuture<void>> futures;

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker([&counter]() { counter.fetch_add(1); });
        if (future.has_value()) futures.push_back(std::move(*future));
    }

    for (auto& f : futures) f.wait();

    EXPECT_EQ(counter.load(), numTasks);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Worker Management Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, DirectTaskAssignment)
{
    std::atomic<bool> executed{false};

    auto future = pool->enqueueToWorkerById(0, [&executed]() { executed.store(true); });

    ASSERT_TRUE(future.has_value());
    future->wait();

    EXPECT_TRUE(executed.load());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Concurrency Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, ConcurrentTaskExecution)
{
    constexpr int numTasks = 1000;
    std::atomic<int> counter{0};
    std::vector<TaskFuture<void>> futures;

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker(
            [&counter]()
            {
                std::this_thread::sleep_for(100us);
                counter.fetch_add(1);
            });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    for (auto& f : futures) f.wait();

    EXPECT_EQ(counter.load(), numTasks);
}

TEST_F(ThreadPoolTest, ParallelAccumulation)
{
    constexpr int numTasks = 100;

    std::vector<int> results(numTasks);
    std::vector<TaskFuture<int>> futures;

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker([i]() { return i * i; });
        if (future.has_value()) futures.push_back(std::move(*future));
    }

    for (size_t i = 0; i < futures.size(); ++i)
    {
        results[i] = futures[i].get();
    }

    int expected = 0;

    for (int i = 0; i < numTasks; ++i) expected += i * i;

    int actual = std::accumulate(results.begin(), results.end(), 0);
    EXPECT_EQ(actual, expected);
}

TEST_F(ThreadPoolTest, RaceConditionDetection)
{
    constexpr int numIterations = 10000;

    std::atomic<int> sharedCounter{0};
    int unsafeCounter = 0;
    std::mutex mutex;

    std::vector<TaskFuture<void>> futures;

    // Safe increments
    for (int i = 0; i < numIterations; ++i)
    {
        auto future = pool->enqueueToWorker([&sharedCounter]() { sharedCounter.fetch_add(1); });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    // Unsafe increments (should still work with proper synchronization)
    for (int i = 0; i < numIterations; ++i)
    {
        auto future = pool->enqueueToWorker(
            [&unsafeCounter, &mutex]()
            {
                std::lock_guard lock(mutex);
                unsafeCounter++;
            });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    for (auto& f : futures) f.wait();

    EXPECT_EQ(sharedCounter.load(), numIterations);
    EXPECT_EQ(unsafeCounter, numIterations);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Work Stealing Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, WorkStealingOccurs)
{
    // Saturate one worker with tasks
    constexpr int numTasks = 100;
    std::atomic<int> counter{0};
    std::vector<TaskFuture<void>> futures;

    // Submit all tasks
    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorkerById(0,
                                                [&counter]()
                                                {
                                                    std::this_thread::sleep_for(10ms);
                                                    counter.fetch_add(1);
                                                });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    // Tasks should complete faster than if only one worker was used
    auto start = std::chrono::steady_clock::now();

    for (auto& f : futures) f.wait();

    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(counter.load(), numTasks);

    // Should be less than 100 * 10ms if work stealing occurred
    EXPECT_LT(elapsed, 500ms);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Edge Cases and Error Handling
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, ExceptionInTask)
{
    std::atomic<bool> nextTaskExecuted{false};

    auto future1 = pool->enqueueToWorker([]() { throw std::runtime_error("Test exception"); });

    auto future2 = pool->enqueueToWorker([&nextTaskExecuted]() { nextTaskExecuted.store(true); });

    // First task should throw
    ASSERT_TRUE(future1.has_value());
    EXPECT_THROW(future1->get(), std::runtime_error);

    // Second task should still execute
    ASSERT_TRUE(future2.has_value());
    future2->wait();

    EXPECT_TRUE(nextTaskExecuted.load());
}

TEST_F(ThreadPoolTest, TaskAfterShutdown)
{
    pool->shutdown();

    auto future = pool->enqueueToWorker([]() { return 42; });

    EXPECT_FALSE(future.has_value()) << "Should not accept tasks after shutdown";
}

TEST_F(ThreadPoolTest, EmptyTask)
{
    auto future = pool->enqueueToWorker([]() {});

    ASSERT_TRUE(future.has_value());
    EXPECT_NO_THROW(future->wait());
}

TEST_F(ThreadPoolTest, HeavyComputationTask)
{
    auto fibonacci = [](int n) -> uint64_t
    {
        if (n <= 1) return n;

        uint64_t a = 0, b = 1;

        for (int i = 2; i <= n; ++i)
        {
            uint64_t temp = a + b;
            a = b;
            b = temp;
        }

        return b;
    };

    auto future = pool->enqueueToWorker(fibonacci, 50);

    ASSERT_TRUE(future.has_value());
    EXPECT_GT(future->get(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Performance and Stress Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, HighThroughput)
{
    constexpr int numTasks = 100000;
    constexpr int workIterations = 10;

    std::atomic<int> counter{0};
    std::vector<TaskFuture<void>> futures;
    futures.reserve(numTasks);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker([&counter, workIterations]()
                                            { counter.fetch_add(1, std::memory_order_relaxed); });

        if (future.has_value()) futures.push_back(std::move(future.value()));
    }

    for (auto& f : futures) f.wait();

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(end - start);

    EXPECT_EQ(counter.load(), numTasks);

    auto throughput = numTasks / elapsed.count();

    std::cout << "Throughput: " << throughput << " tasks/sec\n";
    std::cout << "Total time: " << elapsed.count() * 1000 << " ms\n";

    EXPECT_GT(throughput, 1000);
}

TEST_F(ThreadPoolTest, BurstyLoad)
{
    std::atomic<int> counter{0};

    // Burst 1: Many small tasks
    {
        std::vector<TaskFuture<void>> futures;
        for (int i = 0; i < 500; ++i)
        {
            auto future = pool->enqueueToWorker([&counter]() { counter.fetch_add(1); });
            if (future.has_value()) futures.push_back(std::move(*future));
        }
        for (auto& f : futures) f.wait();
    }

    std::this_thread::sleep_for(50ms);

    // Burst 2: Fewer heavy tasks
    {
        std::vector<TaskFuture<void>> futures;
        for (int i = 0; i < 50; ++i)
        {
            auto future = pool->enqueueToWorker(
                [&counter]()
                {
                    std::this_thread::sleep_for(1ms);
                    counter.fetch_add(1);
                });
            if (future.has_value()) futures.push_back(std::move(*future));
        }
        for (auto& f : futures) f.wait();
    }

    EXPECT_EQ(counter.load(), 550);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Synchronization Primitives Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, LatchSynchronization)
{
    constexpr int numTasks = 10;

    std::latch done{numTasks};
    std::atomic<int> counter{0};

    std::vector<TaskFuture<void>> futures;

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker(
            [&counter, &done]()
            {
                counter.fetch_add(1);
                done.count_down();
            });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    done.wait();

    EXPECT_EQ(counter.load(), numTasks);
}

TEST_F(ThreadPoolTest, BarrierSynchronization)
{
    const int numTasks = std::min(8, static_cast<int>(pool->getWorkersCount()));

    if (numTasks < 2)
    {
        GTEST_SKIP() << "Need at least 2 workers for barrier test";
        return;
    }

    std::barrier sync{numTasks};
    std::atomic<int> phase1{0}, phase2{0};

    std::vector<TaskFuture<void>> futures;

    for (int i = 0; i < numTasks; ++i)
    {
        auto future = pool->enqueueToWorker(
            [&phase1, &phase2, &sync]()
            {
                // Phase 1
                phase1.fetch_add(1);
                sync.arrive_and_wait();

                // Phase 2
                phase2.fetch_add(1);
            });

        if (future.has_value()) futures.push_back(std::move(*future));
    }

    for (auto& f : futures) f.wait();

    EXPECT_EQ(phase1.load(), numTasks);
    EXPECT_EQ(phase2.load(), numTasks);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory and Resource Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ThreadPoolTest, MoveOnlyTypes)
{
    auto ptr = std::make_unique<int>(42);

    auto future = pool->enqueueToWorker([p = std::move(ptr)]() { return *p; });

    ASSERT_TRUE(future.has_value());
    EXPECT_EQ(future->get(), 42);
}

TEST_F(ThreadPoolTest, LargeCapture)
{
    std::vector<int> largeVec(10000, 42);

    auto future = pool->enqueueToWorker([vec = std::move(largeVec)]()
                                        { return std::accumulate(vec.begin(), vec.end(), 0); });

    ASSERT_TRUE(future.has_value());
    EXPECT_EQ(future->get(), 420000);
}
