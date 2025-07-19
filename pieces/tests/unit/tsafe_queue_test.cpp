#include <gtest/gtest.h>
#include <thread>

#include "pieces/tsafe/queue.hpp"

using namespace pieces::tsafe;

/////////////////////////////////////////////////////////////////////////////
//                          Single-threaded tests
/////////////////////////////////////////////////////////////////////////////

TEST(ThreadSafeQueueTest, PushAndTryPop)
{
    ThreadSafeQueue<int> queue;
    queue.push(42);

    auto result = queue.tryPop();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 42);
}

TEST(ThreadSafeQueueTest, TryPopEmptyReturnsNullopt)
{
    ThreadSafeQueue<int> queue;

    // Popping when empty should fail
    EXPECT_FALSE(queue.tryPop().isOk());
}

TEST(ThreadSafeQueueTest, MultiplePushAndPop)
{
    ThreadSafeQueue<int> queue;

    for (int i = 0; i < 10; ++i)
    {
        queue.push(i);
    }

    for (int i = 0; i < 10; ++i)
    {
        auto result = queue.tryPop();
        EXPECT_TRUE(result.isOk());
        EXPECT_EQ(result.unwrap(), i);
    }

    EXPECT_TRUE(queue.empty());
}

/////////////////////////////////////////////////////////////////////////////
//                          Multi-threaded tests
/////////////////////////////////////////////////////////////////////////////

TEST(ThreadSafeQueueTest, WaitAndPopBlocksUntilAvailable)
{
    ThreadSafeQueue<int> queue;
    int poppedValue = 0;

    std::thread producer(
        [&]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            queue.push(1337);
        });

    std::thread consumer([&]() { poppedValue = queue.waitAndPop(); });

    producer.join();
    consumer.join();

    EXPECT_EQ(poppedValue, 1337);
}

TEST(ThreadSafeQueueTest, ThreadedProducerConsumer)
{
    ThreadSafeQueue<int> queue;

    std::thread producer(
        [&]()
        {
            for (int i = 0; i < 100; ++i)
            {
                queue.push(i);
            }
        });

    std::vector<int> consumed;
    std::thread consumer(
        [&]()
        {
            for (int i = 0; i < 100; ++i)
            {
                while (true)
                {
                    auto result = queue.tryPop();

                    if (result.isOk())
                    {
                        consumed.push_back(result.unwrap());
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });

    producer.join();
    consumer.join();

    EXPECT_EQ(consumed.size(), 100);
}
