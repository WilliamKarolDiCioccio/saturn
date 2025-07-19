#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>

#include <pieces/containers/tsafe/deque.hpp>

using namespace pieces::tsafe;

/////////////////////////////////////////////////////////////////////////////
//                          Single-threaded tests
/////////////////////////////////////////////////////////////////////////////

TEST(DequeTest, PushPopSingleThread)
{
    Deque<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);

    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 3u);

    // LIFO order: last in, first out
    auto result = queue.tryPop();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 3);
    EXPECT_EQ(queue.size(), 2u);

    result = queue.tryPop();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 2);

    result = queue.tryPop();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 1);

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);

    // Popping when empty should fail
    EXPECT_FALSE(queue.tryPop().isOk());
}

TEST(DequeTest, SingleThreadSteal)
{
    Deque<int> queue;
    EXPECT_TRUE(queue.empty());

    queue.push(10);
    queue.push(20);
    queue.push(30);

    // FIFO order: first in, first out
    auto result = queue.trySteal();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 10);
    EXPECT_EQ(queue.size(), 2u);

    result = queue.trySteal();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 20);

    result = queue.trySteal();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 30);

    EXPECT_TRUE(queue.empty());

    // Stealing when empty should fail
    EXPECT_FALSE(queue.trySteal().isOk());
}

/////////////////////////////////////////////////////////////////////////////
//                          Multi-threaded tests
/////////////////////////////////////////////////////////////////////////////

TEST(DequeTest, ConcurrentSteal)
{
    constexpr int kNumItems = 1000;
    constexpr int kNumThreads = 4;
    Deque<int> queue;

    // Fill the queue with 0..kNumItems-1
    for (int i = 0; i < kNumItems; ++i)
    {
        queue.push(i);
    }

    EXPECT_EQ(queue.size(), static_cast<size_t>(kNumItems));

    std::vector<int> stolen;
    stolen.reserve(kNumItems);
    std::mutex stolen_mutex;

    // Launch threads to steal
    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back(
            [&]()
            {
                while (true)
                {
                    auto result = queue.trySteal();

                    if (result.isErr()) break;

                    std::lock_guard<std::mutex> lock(stolen_mutex);
                    stolen.push_back(result.unwrap());
                }
            });
    }

    // Wait for all threads
    for (auto& th : threads)
    {
        th.join();
    }

    // After all stealing, queue should be empty
    EXPECT_TRUE(queue.empty());

    // All items should have been stolen
    EXPECT_EQ(stolen.size(), static_cast<size_t>(kNumItems));

    // Verify that stolen contains exactly the numbers 0..kNumItems-1
    std::sort(stolen.begin(), stolen.end());
    for (int i = 0; i < kNumItems; ++i)
    {
        EXPECT_EQ(stolen[i], i);
    }
}
