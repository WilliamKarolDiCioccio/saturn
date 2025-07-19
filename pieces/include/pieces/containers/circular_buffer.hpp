#pragma once

#include <array>
#include <optional>
#include <stdexcept>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{

/**
 * @brief A vector with a fixed size that discards the oldest element when full.
 *
 * This class implements a vector that has a fixed size. When the queue is full and a new element is
 * added, the oldest element is removed to make space for the new one. This is useful for scenarios
 * where you want to keep only the most recent elements in memory. This is also known as a
 * circular buffer.
 *
 * @note The class provides trivial wrappers for the STL vector methods, so you can use it both
 * as a queue and as a vector.
 *
 * @tparam T The type of the elements in the queue.
 */
template <typename T, size_t Size>
    requires std::is_default_constructible_v<T> && (Size > 1)
class CircularBuffer
{
   private:
    std::array<T, Size> m_data;
    std::size_t m_size = 0;
    std::size_t m_start = 0;

    // Helper function to convert logical index to physical index
    std::size_t getPhysicalIndex(std::size_t _logicalIndex) const
    {
        return (m_start + _logicalIndex) % Size;
    }

   public:
    /**
     * @brief Constructs a CircularBuffer with the specified capacity.
     *
     * You won't be able to change capacity after the queue is created.
     *
     * @param _capacity The maximum number of elements the queue can hold.
     * @throws std::invalid_argument if _capacity is 0.
     */
    CircularBuffer() = default;

    /**
     * @brief Adds an element to the front of the queue.
     *
     * @param _value The value to add to the queue.
     */
    void push(const T& _value)
    {
        if (m_size == Size)
        {
            // Buffer is full, overwrite the oldest element
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = _value;
        }
        else
        {
            // Buffer not full, add to front
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = _value;
            ++m_size;
        }
    }

    /**
     * @brief Adds an element to the front of the queue using move semantics.
     *
     * @param _value The rvalue reference to add to the queue.
     */
    void push(T&& _value)
    {
        if (m_size == Size)
        {
            // Buffer is full, overwrite the oldest element
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = std::move(_value);
        }
        else
        {
            // Buffer not full, add to front
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = std::move(_value);
            ++m_size;
        }
    }

    /**
     * @brief Builds and adds an element to the front of the queue.
     *
     * @param _value The value to add to the queue.
     */
    void emplace(const T& _value)
    {
        if (m_size == Size)
        {
            // Buffer is full, overwrite the oldest element
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = _value;
        }
        else
        {
            // Buffer not full, add to front
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = _value;
            ++m_size;
        }
    }

    /**
     * @brief Builds and adds an element to the front of the queue using move semantics.
     *
     * @param _value The rvalue reference to add to the queue.
     */
    void emplace(T&& _value)
    {
        if (m_size == Size)
        {
            // Buffer is full, overwrite the oldest element
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = std::move(_value);
        }
        else
        {
            // Buffer not full, add to front
            m_start = (m_start + Size - 1) % Size;
            m_data[m_start] = std::move(_value);
            ++m_size;
        }
    }

    /**
     * @brief Removes the oldest element from the queue and returns it.
     *
     * @return Result containing the popped value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] Result<T, ErrorCode> pop()
    {
        if (empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        auto out = std::move(m_data[m_start]);
        m_start = (m_start + 1) % Size;
        --m_size;

        return Ok<T, ErrorCode>(std::move(out));
    }

    /**
     * @brief Removes the oldest element from the queue.
     *
     * @return true if an element was removed, false if the queue was empty.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] Result<T, ErrorCode> front() const
    {
        if (empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        return Ok<T, ErrorCode>(T(m_data[m_start]));
    }

    /**
     * @brief Returns the last element in the queue.
     *
     * @return The last element in the queue.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] Result<T, ErrorCode> back() const
    {
        if (empty()) return Err<T, ErrorCode>(ErrorCode::container_empty);

        std::size_t backIndex = (m_start + m_size - 1) % Size;

        return Ok<T, ErrorCode>(T(m_data[backIndex]));
    }

    [[nodiscard]] std::size_t size() const { return m_size; }
    [[nodiscard]] std::size_t capacity() const { return Size; }

    bool empty() const { return m_size == 0; }
    void clear()
    {
        m_size = 0;
        m_start = 0;
    }

    [[nodiscard]] auto begin() { return m_data.begin(); }
    [[nodiscard]] auto end() { return m_data.end(); }
    [[nodiscard]] auto begin() const { return m_data.begin(); }
    [[nodiscard]] auto end() const { return m_data.end(); }

    [[nodiscard]] auto data() { return m_data.data(); }
    [[nodiscard]] auto data() const { return m_data.data(); }

    /**
     * @brief Access an element at a specific index.
     *
     * @param _index The index of the element to access.
     * @return const T& The element at the specified index.
     * @throws std::out_of_range if the index is out of range or the queue is empty.
     */
    const T& operator[](size_t _index) const
    {
        if (_index >= m_size)
        {
            throw std::out_of_range("Index out of range");
        }

        return m_data[getPhysicalIndex(_index)];
    }

    /**
     * @brief Non-const vsion of the operator[] to access an element at a specific index.
     */
    T& operator[](size_t _index)
    {
        if (_index >= m_size)
        {
            throw std::out_of_range("Index out of range");
        }

        return m_data[getPhysicalIndex(_index)];
    }
};

} // namespace pieces
