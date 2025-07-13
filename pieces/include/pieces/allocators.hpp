#pragma once

#include <cstddef>
#include <stdexcept>
#include <new>
#include <cstring>
#include <type_traits>

#include "templates.hpp"

namespace pieces
{

/**
 * @brief Concept for a generic allocator.
 *
 * This concept checks if the type `A` meets the requirements of an allocator,
 * including allocation, deallocation, construction, destruction, and capacity management.
 */
template <typename A>
concept Allocator = requires(A a, size_t n) {
    { a.allocate(n) } -> std::same_as<typename A::value_type*>;
    { a.deallocate(nullptr, n) };
    {
        a.construct(std::declval<typename A::value_type*>(), std::declval<typename A::value_type>())
    };
    { a.destroy(std::declval<typename A::value_type*>()) };
    { a.reset() };
    { a.capacity() } -> std::same_as<size_t>;
    { a.used() } -> std::same_as<size_t>;
    { a.available() } -> std::same_as<size_t>;
    { a == a } -> std::same_as<bool>;
    { a != a } -> std::same_as<bool>;
};

/**
 * @brief A linear allocator that allocates memory in a contiguous block.
 *
 * This allocator is designed for performance in scenarios where memory is allocated
 * and deallocated in a linear fashion. It does not support deallocation of individual objects,
 * but rather manages a single block of memory that can be reused.
 *
 * @tparam T The type of objects to allocate memory for.
 */
template <typename T>
class LinearAllocator final : public NonCopyable
{
   private:
    char* m_buffer = nullptr;
    size_t m_capacity = 0;
    size_t m_offset = 0;

   public:
    explicit LinearAllocator(size_t _capacity) : m_capacity(_capacity), m_offset(0)
    {
        if (_capacity == 0) throw std::invalid_argument("Size must be greater than zero.");

        size_t bytesNeeded = _capacity * sizeof(T);

        m_buffer = new char[bytesNeeded];

        if (!m_buffer) throw std::bad_alloc();
    }

    ~LinearAllocator() { delete[] m_buffer; }

    LinearAllocator(LinearAllocator&& _other) noexcept
        : m_buffer(_other.m_buffer), m_capacity(_other.m_capacity), m_offset(_other.m_offset)
    {
        _other.m_buffer = nullptr;
        _other.m_capacity = 0;
        _other.m_offset = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& _other) noexcept
    {
        if (this == &_other) return *this;

        delete[] m_buffer;
        m_buffer = _other.m_buffer;
        m_capacity = _other.m_capacity;
        m_offset = _other.m_offset;
        _other.m_buffer = nullptr;
        _other.m_capacity = 0;
        _other.m_offset = 0;

        return *this;
    }

   public:
    T* allocate(size_t _count)
    {
        if (_count == 0) return nullptr;

        size_t alignedOffset = alignOffset(m_offset);
        size_t bytesNeeded = _count * sizeof(T);
        size_t capacityInBytes = m_capacity * sizeof(T);

        if (alignedOffset + bytesNeeded > capacityInBytes) return nullptr;

        T* ptr = reinterpret_cast<T*>(m_buffer + alignedOffset);

        m_offset = alignedOffset + bytesNeeded;

        return ptr;
    }

    void deallocate([[maybe_unused]] void* _ptr, [[maybe_unused]] size_t _count) noexcept
    {
        // No-op for linear allocators
    }

    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args)
    {
        if (!_ptr) throw std::invalid_argument("Pointer cannot be null.");
        ::new (static_cast<void*>(_ptr)) U(std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept
    {
        if (_ptr) _ptr->~U();
    }

    void reset() { m_offset = 0; }

    size_t capacity() const noexcept { return m_capacity; }

    size_t used() const noexcept { return m_offset; }

    size_t available() const noexcept { return m_capacity - m_offset; }

    bool operator==(const LinearAllocator& other) const noexcept
    {
        return m_buffer == other.m_buffer && m_capacity == other.m_capacity &&
               m_offset == other.m_offset;
    }

    bool operator!=(const LinearAllocator& other) const noexcept { return !(*this == other); }

   private:
    size_t alignOffset(size_t _offset) const noexcept
    {
        constexpr size_t alignment = alignof(T);
        return (_offset + alignment - 1) & ~(alignment - 1);
    }
};

} // namespace pieces
