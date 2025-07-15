#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>

#include "pieces/templates.hpp"

namespace pieces
{

/**
 * @brief A linear allocator that allocates memory in a contiguous block.
 *
 * This allocator is designed for performance in scenarios where memory is allocated
 * in a linear fashion and deallocated all at once.
 *
 * @tparam T The type of objects to allocate memory for.
 */
template <typename T>
    requires std::is_trivially_destructible_v<T>
class LinearAllocator final : public NonCopyable
{
   public:
    using ValueType = T;

   private:
    char* m_bufferBytes = nullptr;
    size_t m_capacity = 0;
    size_t m_offsetInBytes = 0;

   public:
    explicit LinearAllocator(size_t _capacity) : m_capacity(_capacity), m_offsetInBytes(0)
    {
        if (_capacity == 0) throw std::invalid_argument("Size must be greater than zero.");

        size_t bytesNeeded = _capacity * sizeof(T);

        m_bufferBytes = ::new char[bytesNeeded];
    }

    ~LinearAllocator() { delete[] m_bufferBytes; }

    LinearAllocator(LinearAllocator&& _other) noexcept
        : m_bufferBytes(_other.m_bufferBytes),
          m_capacity(_other.m_capacity),
          m_offsetInBytes(_other.m_offsetInBytes)
    {
        _other.m_bufferBytes = nullptr;
        _other.m_capacity = 0;
        _other.m_offsetInBytes = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& _other) noexcept
    {
        if (this == &_other) return *this;

        delete[] m_bufferBytes;

        m_bufferBytes = _other.m_bufferBytes;
        m_capacity = _other.m_capacity;
        m_offsetInBytes = _other.m_offsetInBytes;

        _other.m_bufferBytes = nullptr;
        _other.m_capacity = 0;
        _other.m_offsetInBytes = 0;

        return *this;
    }

   public:
    T* allocate(size_t _count)
    {
        if (_count == 0) return nullptr;

        size_t bytesNeeded = _count * sizeof(T);
        size_t capacityInBytes = m_capacity * sizeof(T);
        size_t remainingBytes = capacityInBytes - m_offsetInBytes;

        void* rawPtr = static_cast<void*>(m_bufferBytes + m_offsetInBytes);
        void* alignedPtr = rawPtr;

        if (!std::align(alignof(T), bytesNeeded, alignedPtr, remainingBytes)) return nullptr;

        m_offsetInBytes = static_cast<char*>(alignedPtr) - m_bufferBytes + bytesNeeded;

        return static_cast<T*>(alignedPtr);
    }

    void deallocate([[maybe_unused]] T* _ptr, [[maybe_unused]] size_t _count) noexcept
    {
        // No-op for linear allocators
    }

    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args)
    {
        if (!owns(_ptr)) return;

        ::new (static_cast<void*>(_ptr)) U(std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept
    {
        if (owns(_ptr)) _ptr->~U();
    }

    void reset()
    {
        m_offsetInBytes = 0;
        size_t capacityInBytes = m_capacity * sizeof(T);
        std::memset(m_bufferBytes, 0, capacityInBytes);
    }

    size_t capacity() const noexcept { return m_capacity; }

    size_t used() const noexcept { return m_offsetInBytes; }

    size_t available() const noexcept { return m_capacity - m_offsetInBytes; }

    bool owns(void* _ptr) const noexcept
    {
        if (!_ptr) return false;

        char* charPtr = reinterpret_cast<char*>(_ptr);

        if (charPtr < m_bufferBytes) return false;

        size_t ptrOffsetInBytes = charPtr - m_bufferBytes;
        return ptrOffsetInBytes < m_offsetInBytes;
    }

    bool operator==(const LinearAllocator& other) const noexcept
    {
        return this == &other || m_bufferBytes == other.m_bufferBytes &&
                                     m_capacity == other.m_capacity &&
                                     m_offsetInBytes == other.m_offsetInBytes;
    }

    bool operator!=(const LinearAllocator& other) const noexcept { return !(*this == other); }
};

} // namespace pieces
