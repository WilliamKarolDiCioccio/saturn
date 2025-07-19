#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{
namespace tsafe
{

/**
 * @brief Thread-safe queue wrapper around std::queue.
 *
 * This class provides a thread-safe interface for inserting and removing
 * elements from a queue. It uses a mutex and condition variable to ensure
 * safe access from multiple threads. The object is not copyable or movable.
 *
 * @tparam T Type of elements in the queue.
 */
template <typename T>
class ThreadSafeQueue
{
   private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condVar;

   public:
    ThreadSafeQueue() = default;

   public:
    /**
     * @brief Push an element to the back of the queue.
     *
     * @param _value The value to be pushed into the queue.
     */
    void push(const T& _value)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(_value);
        }
        m_condVar.notify_one();
    }

    /**
     * @brief Push a value to the back of the queue using move semantics.
     *
     * @param _value The rvalue reference to be pushed into the queue.
     */
    void emplace(T&& _value)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(_value));
        }

        m_condVar.notify_one();
    }

    /**
     * @brief Wait for an element to be available and pop it from the queue.
     *
     * @return T The popped value.
     */
    [[nodiscard]] T waitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_condVar.wait(lock, [this] { return !m_queue.empty(); });

        auto result = std::move(m_queue.front());

        m_queue.pop();

        return result;
    }

    /**
     * @brief Try to pop an element from the queue without blocking.
     *
     * @return Result containing the popped value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] Result<T, ErrorCode> tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        auto value = std::move(m_queue.front());

        m_queue.pop();

        return Ok<T, ErrorCode>(std::move(value));
    }

    [[nodiscard]] bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    [[nodiscard]] size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
};

} // namespace tsafe
} // namespace pieces
