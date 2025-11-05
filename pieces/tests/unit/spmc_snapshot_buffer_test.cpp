#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <algorithm>

#include <pieces/containers/spmc_snapshot_buffer.hpp>

using namespace pieces;
using namespace std::chrono_literals;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture
////////////////////////////////////////////////////////////////////////////////////////////////////

class SPMCSnapshotBufferTest : public ::testing::Test
{
   protected:
    SPMCSnapshotBuffer<int> m_buffer;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Snapshot Buffer Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(SPMCSnapshotBufferTest, SingleProducerSingleConsumer)
{
    // Producer writes data
    for (int i = 0; i < 100; ++i) m_buffer.write(i);

    EXPECT_EQ(m_buffer.pendingSize(), 100);
    EXPECT_TRUE(m_buffer.hasPending());

    // Publish snapshot
    EXPECT_TRUE(m_buffer.publish());
    EXPECT_EQ(m_buffer.pendingSize(), 0);
    EXPECT_FALSE(m_buffer.hasPending());

    // Consumer reads snapshot
    auto snapshot = m_buffer.getSnapshot();
    EXPECT_EQ(snapshot->size(), 100);

    // Verify data integrity
    for (int i = 0; i < 100; ++i) EXPECT_EQ((*snapshot)[i], i);
}

TEST_F(SPMCSnapshotBufferTest, SingleProducerMultipleConsumers)
{
    constexpr int numItems = 1000;
    constexpr int numConsumers = 5;

    // Producer writes data
    for (int i = 0; i < numItems; ++i) m_buffer.write(i);

    // Publish snapshot
    EXPECT_TRUE(m_buffer.publish());

    // Multiple consumers read concurrently
    std::vector<std::thread> consumers;
    std::vector<bool> success(numConsumers);

    for (int c = 0; c < numConsumers; ++c)
    {
        consumers.emplace_back(
            [this, &success, c, numItems]()
            {
                auto snapshot = m_buffer.getSnapshot();

                // Verify size
                if (snapshot->size() != numItems)
                {
                    success[c] = false;
                    return;
                }

                // Verify data integrity
                for (int i = 0; i < numItems; ++i)
                {
                    if ((*snapshot)[i] != i)
                    {
                        success[c] = false;
                        return;
                    }
                }

                success[c] = true;
            });
    }

    // Wait for all consumers
    for (auto& t : consumers) t.join();

    // All consumers should have read successfully
    for (int c = 0; c < numConsumers; ++c)
    {
        EXPECT_TRUE(success[c]) << "Consumer " << c << " failed";
    }
}

TEST_F(SPMCSnapshotBufferTest, DifferentDataTypes)
{
    struct LogEntry
    {
        int id;
        std::string message;
        double timestamp;

        bool operator==(const LogEntry& other) const
        {
            return id == other.id && message == other.message && timestamp == other.timestamp;
        }
    };

    SPMCSnapshotBuffer<LogEntry> buffer;

    std::vector<LogEntry> entries = {
        {1, "Start", 1.0}, {2, "Processing", 2.5}, {3, "Complete", 3.7}};

    for (const auto& entry : entries) buffer.write(entry);

    buffer.publish();
    auto snapshot = buffer.getSnapshot();

    ASSERT_EQ(snapshot->size(), entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
    {
        EXPECT_EQ((*snapshot)[i], entries[i]);
    }
}

TEST_F(SPMCSnapshotBufferTest, EmptyBufferPublish)
{
    // Publishing empty buffer should return false
    EXPECT_FALSE(m_buffer.publish());

    auto snapshot = m_buffer.getSnapshot();
    EXPECT_TRUE(snapshot->empty());
    EXPECT_EQ(snapshot->size(), 0);
}

TEST_F(SPMCSnapshotBufferTest, MultiplePublishCycles)
{
    for (int cycle = 0; cycle < 5; ++cycle)
    {
        // Write data for this cycle
        int start = cycle * 100;
        for (int i = 0; i < 100; ++i) m_buffer.write(start + i);

        EXPECT_TRUE(m_buffer.publish());

        auto snapshot = m_buffer.getSnapshot();
        ASSERT_EQ(snapshot->size(), 100);

        // Verify data for this cycle
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_EQ((*snapshot)[i], start + i);
        }
    }
}

TEST_F(SPMCSnapshotBufferTest, IteratorSupport)
{
    for (int i = 0; i < 10; ++i)
    {
        m_buffer.write(i * 2);
    }

    m_buffer.publish();
    auto snapshot = m_buffer.getSnapshot();

    // Test range-based for loop
    int sum = 0;
    for (const auto& val : *snapshot)
    {
        sum += val;
    }

    // Expected sum: 0 + 2 + 4 + 6 + 8 + 10 + 12 + 14 + 16 + 18 = 90
    EXPECT_EQ(sum, 90);

    // Test with standard algorithms
    auto it = std::find(snapshot->begin(), snapshot->end(), 10);
    EXPECT_NE(it, snapshot->end());
    EXPECT_EQ(*it, 10);
}

TEST_F(SPMCSnapshotBufferTest, InterleavedWriteAndPublish)
{
    std::atomic<bool> done{false};
    std::atomic<int> readCount{0};

    std::thread writer(
        [&]()
        {
            for (int i = 0; i < 1000; ++i)
            {
                m_buffer.write(i);
                if (i % 50 == 0)
                {
                    m_buffer.publish();
                }
            }
            done.store(true);
        });

    std::thread reader(
        [&]()
        {
            while (!done.load())
            {
                auto snapshot = m_buffer.getSnapshot();
                readCount++;

                // Just verify snapshot is internally consistent
                if (!snapshot->empty())
                {
                    for (size_t i = 1; i < snapshot->size(); ++i)
                    {
                        EXPECT_GE((*snapshot)[i], (*snapshot)[i - 1]);
                    }
                }
            }
        });

    writer.join();
    reader.join();

    EXPECT_GT(readCount.load(), 0);
}

TEST_F(SPMCSnapshotBufferTest, SnapshotConsistencyUnderLoad)
{
    constexpr int num_cycles = 100;
    std::atomic<int> current_cycle{0};

    std::thread producer(
        [&]()
        {
            for (int cycle = 0; cycle < num_cycles; ++cycle)
            {
                m_buffer.write(cycle); // Write cycle marker
                for (int i = 0; i < 10; ++i)
                {
                    m_buffer.write(cycle * 100 + i);
                }
                current_cycle.store(cycle);
                m_buffer.publish();
            }
        });

    std::thread verifier(
        [&]()
        {
            for (int i = 0; i < num_cycles * 2; ++i)
            {
                auto snapshot = m_buffer.getSnapshot();
                if (snapshot->size() >= 2)
                { // At least cycle marker + one value
                    int cycle_marker = (*snapshot)[0];
                    // All values should be from the same or consecutive cycles
                    for (size_t j = 1; j < snapshot->size(); ++j)
                    {
                        int value = (*snapshot)[j];
                        int value_cycle = value / 100;
                        EXPECT_GE(value_cycle, cycle_marker - 1); // Allow for slight staleness
                        EXPECT_LE(value_cycle, current_cycle.load());
                    }
                }
            }
        });

    producer.join();
    verifier.join();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Stress Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(SPMCSnapshotBufferTest, StressTestHighContention)
{
    constexpr int numConsumers = 20;
    constexpr int numOperations = 1000;
    constexpr int publishInterval = 10;

    std::atomic<bool> done{false};
    std::atomic<int> publishedCount{0};

    // Single producer thread
    std::thread producer(
        [&]()
        {
            for (int i = 0; i < numOperations; ++i)
            {
                m_buffer.write(i);
                if (i % publishInterval == 0)
                {
                    if (m_buffer.publish())
                    {
                        publishedCount++;
                    }
                }
            }
            // Final publish
            if (m_buffer.publish())
            {
                publishedCount++;
            }
            done.store(true, std::memory_order_release);
        });

    // Many consumer threads reading snapshots concurrently
    std::vector<std::thread> consumers;
    std::atomic<int> totalReads{0};

    for (int c = 0; c < numConsumers; ++c)
    {
        consumers.emplace_back(
            [&]()
            {
                int local_reads = 0;
                // Run until producer is done and has published data
                while (!done.load(std::memory_order_acquire))
                {
                    auto snapshot = m_buffer.getSnapshot();
                    if (!snapshot->empty())
                    {
                        local_reads++;
                    }
                    std::this_thread::yield();
                }

                // One final read after producer completes
                auto snapshot = m_buffer.getSnapshot();
                if (!snapshot->empty())
                {
                    local_reads++;
                }

                totalReads.fetch_add(local_reads, std::memory_order_relaxed);
            });
    }

    producer.join();
    for (auto& t : consumers)
    {
        t.join();
    }

    // Should have completed without crashes or deadlocks
    EXPECT_GT(publishedCount.load(), 0);
    EXPECT_GT(totalReads.load(), 0);

    // Final snapshot should contain the last batch
    auto finalSnapshot = m_buffer.getSnapshot();
    EXPECT_FALSE(finalSnapshot->empty());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Registry Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(SPMCSnapshotBufferTest, RegistryMultipleThreads)
{
    SPMCSnapshotBufferRegistry<int> registry;
    constexpr int num_threads = 4;
    constexpr int items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [&registry, t, items_per_thread]()
            {
                auto buffer = std::make_shared<SPMCSnapshotBuffer<int>>();
                registry.registerBuffer(buffer);

                // Each thread writes its own range
                int start = t * items_per_thread;
                for (int i = 0; i < items_per_thread; ++i)
                {
                    buffer->write(start + i);
                }

                buffer->publish();
            });
    }

    for (auto& t : threads) t.join();

    // Collect all snapshots
    auto snapshots = registry.collectAllSnapshots();
    EXPECT_EQ(snapshots.size(), num_threads);

    // Verify total count
    size_t total_items = 0;
    for (const auto& snapshot : snapshots)
    {
        total_items += snapshot.size();
    }

    EXPECT_EQ(total_items, num_threads * items_per_thread);
}
