#pragma once

#include <deque>
#include <mutex>
#include <optional>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{
namespace tsafe
{

/**
 * @brief A thread-safe double-ended queue (deque) implementation.
 *
 * This class provides a thread-safe interface for inserting and removing
 * elements from a deque. It uses a mutex to ensure safe access from multiple threads.
 *
 * @tparam T Type of elements in the queue.
 */
template <typename T>
class Deque
{
   private:
    mutable std::mutex m_mutex;
    std::deque<T> m_deque;

   public:
    Deque() = default;
    Deque(const Deque&) = delete;
    Deque& operator=(const Deque&) = delete;

    /**
     * @brief Push an element to the back of the queue.
     *
     * @param _value The value to be pushed into the queue.
     */
    void push(const T& _value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_deque.push_back(_value);
    }

    /**
     * @brief Push a value to the back of the queue using move semantics.
     *
     * @param _value The rvalue reference to be pushed into the queue.
     */
    void emplace(T&& _value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_deque.emplace_back(std::forward<T>(_value));
    }

    /**
     * @brief Try to pop an element from the back of the queue (LIFO).
     *
     * @return Result containing the popped value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    Result<T, ErrorCode> tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_deque.empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        auto result = std::move(m_deque.back());

        m_deque.pop_back();

        return Ok<T, ErrorCode>(std::move(result));
    }

    /**
     * @brief Try to pop an element from the front of the queue (FIFO).
     *
     * @return Result containing the stolen value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    Result<T, ErrorCode> trySteal()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_deque.empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        auto result = std::move(m_deque.front());

        m_deque.pop_front();

        return Ok<T, ErrorCode>(std::move(result));
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_deque.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_deque.size();
    }
};

} // namespace tsafe
} // namespace pieces
