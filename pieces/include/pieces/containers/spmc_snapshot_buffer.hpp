#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <cstring>

namespace pieces
{

/**
 * @brief A single-producer, multiple-consumer (SPMC) snapshot buffer implementation.
 *
 * A lock-free buffer for single-producer scenarios with thread-safe snapshot
 * publishing for multiple consumers. Each thread maintains its own buffer and
 * can publish immutable snapshots that are accessible to all threads.
 *
 * This class allows:
 * - Lock-free writes for the producer thread
 * - Thread-safe snapshot publishing with double buffering
 * - Immutable snapshot views for concurrent readers
 * - Template-based for type flexibility
 *
 * @tparam T Type of items stored in the buffer
 *
 * @note write(), publish(), pendingSize(), hasPending(), and clear()
 * should only be called from the producer thread. getSnapshot() is safe to
 * call from any thread.
 */
template <typename T>
class SPMCSnapshotBuffer
{
   public:
    /**
     * Immutable snapshot view of buffer contents
     */
    class Snapshot
    {
       private:
        std::vector<T> m_data;

       public:
        Snapshot() = default;

        Snapshot(const std::vector<T>& _data) : m_data(_data) {}

        const std::vector<T>& data() const { return m_data; }

        size_t size() const { return m_data.size(); }

        bool empty() const { return m_data.empty(); }

        const T& operator[](size_t _idx) const { return m_data[_idx]; }

        auto begin() const { return m_data.begin(); }
        auto end() const { return m_data.end(); }
    };

   private:
    size_t m_reserveSize;

    // Thread-local write buffer (single producer, no locking needed)
    std::vector<T> m_writeBuffer;

    // Shared snapshot pointer protected by mutex
    mutable std::mutex m_snapshotMutex;
    std::shared_ptr<Snapshot> m_currentSnapshot;

    // Atomic flag indicating if any data has been published
    std::atomic<bool> m_hasData;

   public:
    explicit SPMCSnapshotBuffer(size_t _reserveSize = 1024)
        : m_reserveSize(_reserveSize),
          m_currentSnapshot(std::make_shared<Snapshot>()),
          m_hasData(false)
    {
        m_writeBuffer.reserve(_reserveSize);
    }

   public:
    /**
     * Write an item to the thread-local buffer (lock-free for single producer)
     * @note MUST be called only from the producer thread
     */
    void write(const T& _item) { m_writeBuffer.push_back(_item); }

    /**
     * Write an item to the thread-local buffer (move version)
     * @note MUST be called only from the producer thread
     */
    void write(T&& _item) { m_writeBuffer.push_back(std::move(_item)); }

    /**
     * Publish current write buffer as a new snapshot
     * @note MUST be called only from the producer thread
     *
     * @return true if there was data to publish, false if buffer was empty
     */
    bool publish()
    {
        if (m_writeBuffer.empty())
        {
            return false;
        }

        // Create new snapshot from write buffer
        auto new_snapshot = std::make_shared<Snapshot>(m_writeBuffer);

        // Thread-safe swap of the snapshot pointer
        {
            std::lock_guard<std::mutex> lock(m_snapshotMutex);
            m_currentSnapshot = new_snapshot;
            m_hasData.store(true, std::memory_order_release);
        }

        // Clear write buffer for next batch
        m_writeBuffer.clear();
        m_writeBuffer.reserve(m_reserveSize);

        return true;
    }

    /**
     * Get the current snapshot (thread-safe, returns immutable view)
     * Multiple consumers can call this concurrently
     */
    std::shared_ptr<Snapshot> getSnapshot() const
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        return m_currentSnapshot;
    }

    /**
     * Check if any data has ever been published
     * Thread-safe, can be called from any thread
     */
    bool hasData() const { return m_hasData.load(std::memory_order_acquire); }

    /**
     * Get current write buffer size (for monitoring/debugging)
     * @note MUST be called only from the producer thread
     */
    size_t pendingSize() const { return m_writeBuffer.size(); }

    /**
     * Check if write buffer has pending data
     * @note MUST be called only from the producer thread
     */
    bool hasPending() const { return !m_writeBuffer.empty(); }

    /**
     * Clear the write buffer without publishing
     * @note MUST be called only from the producer thread
     */
    void clear() { m_writeBuffer.clear(); }
};

/**
 * Registry for managing multiple thread-local buffers
 * Useful for collecting snapshots from all threads
 */
template <typename T>
class SPMCSnapshotBufferRegistry
{
   public:
    using BufferPtr = std::shared_ptr<SPMCSnapshotBuffer<T>>;

   private:
    mutable std::mutex m_registryMutex;
    std::vector<BufferPtr> m_buffers;

   public:
    /**
     * Register a buffer for a thread
     */
    void registerBuffer(BufferPtr _buffer)
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        m_buffers.push_back(_buffer);
    }

    /**
     * Get all registered buffers
     */
    std::vector<BufferPtr> getAllBuffers() const
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        return m_buffers;
    }

    /**
     * Collect snapshots from all registered buffers
     */
    std::vector<typename SPMCSnapshotBuffer<T>::Snapshot> collectAllSnapshots() const
    {
        std::vector<BufferPtr> buffers;
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            buffers = m_buffers;
        }

        std::vector<typename SPMCSnapshotBuffer<T>::Snapshot> snapshots;
        snapshots.reserve(buffers.size());

        for (const auto& buffer : buffers)
        {
            auto snapshot = buffer->getSnapshot();
            if (!snapshot->empty())
            {
                snapshots.push_back(*snapshot);
            }
        }

        return snapshots;
    }

    /**
     * Publish all registered buffers
     */
    size_t publishAll()
    {
        std::vector<BufferPtr> buffers;
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            buffers = m_buffers;
        }

        size_t published_count = 0;
        for (auto& buffer : buffers)
        {
            if (buffer->publish())
            {
                ++published_count;
            }
        }

        return published_count;
    }
};

} // namespace pieces
