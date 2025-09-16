#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>

#include "pieces/core/templates.hpp"
#include "pieces/containers/bitset.hpp"

namespace pieces
{

/**
 * @brief An enum class to define the policy for pool allocators.
 */
enum class PoolAllocatorPolicy : uint8_t
{
    automatic_indexing,
    manual_indexing,
};

/**
 * @brief A pool allocator for managing memory for a fixed number of objects.
 *
 * A pool allocator allows for random access allocation and deallocation of objects
 * within a fixed-size buffer.
 *
 * @tparam T The type of objects to allocate memory for.
 *
 * @note This allocator operates in base T for size, alignment, and all related operations.
 * @note Due to the performance-critical nature of this allocator, it does not support
 * non-trivially-destructible types.
 */
template <typename T, PoolAllocatorPolicy Policy = PoolAllocatorPolicy::automatic_indexing>
class PoolAllocator final : public NonCopyable
{
   public:
    using Byte = uint8_t;
    using ValueType = T;

   private:
    static constexpr size_t ALIGNOF_VALUE = alignof(ValueType);
    static constexpr size_t SIZEOF_VALUE = sizeof(ValueType);

    Byte* m_bufferBytes;
    BitSet m_slotsState;

    // Expressed in Ts
    size_t m_capacity;
    size_t m_size;

   public:
    /**
     * @brief Constructs a PoolAllocator with a given capacity in T objects.
     *
     * @param _capacity The number of T objects the allocator can hold.
     *
     * @throws std::invalid_argument if _capacity is zero.
     */
    explicit PoolAllocator(size_t _capacity)
        : m_bufferBytes(nullptr), m_slotsState(_capacity), m_capacity(_capacity), m_size(0)
    {
        if (_capacity == 0) throw std::invalid_argument("Capacity must be greater than zero.");

        size_t bytesNeeded = _capacity * sizeof(ValueType);

        m_bufferBytes =
            static_cast<Byte*>(::operator new(bytesNeeded, std::align_val_t{alignof(ValueType)}));

        std::memset(m_bufferBytes, 0, bytesNeeded);
    }

    ~PoolAllocator() { ::operator delete(m_bufferBytes, std::align_val_t{alignof(ValueType)}); }

    PoolAllocator(PoolAllocator&& _other) noexcept
        : m_bufferBytes(_other.m_bufferBytes),
          m_slotsState(std::move(_other.m_slotsState)),
          m_capacity(_other.m_capacity),
          m_size(_other.m_size)
    {
        _other.m_bufferBytes = nullptr;
        _other.m_capacity = 0;
        _other.m_size = 0;
    }

    PoolAllocator& operator=(PoolAllocator&& _other) noexcept
    {
        if (this == &_other) return *this;

        ::operator delete(m_bufferBytes, std::align_val_t{alignof(ValueType)});

        m_bufferBytes = _other.m_bufferBytes;
        m_slotsState = std::move(_other.m_slotsState);
        m_capacity = _other.m_capacity;
        m_size = _other.m_size;

        _other.m_bufferBytes = nullptr;
        _other.m_capacity = 0;
        _other.m_size = 0;

        return *this;
    }

   public:
    /**
     * @brief Allocates memory for a given number of T objects.
     *
     * @param _count The number of T objects to allocate memory for.
     * @return Pointer to the allocated memory, or nullptr if allocation fails.
     *
     * @throws std::bad_alloc from the new operator if allocation fails.
     */
    ValueType* allocate(size_t _count)
    {
        if (_count == 0 || _count > m_capacity - m_size) return nullptr;

        if (_count > 1) return allocateContiguous(_count);

        size_t slotIndex = m_slotsState.findFirstClear();
        if (slotIndex >= m_capacity) return nullptr;

        m_slotsState.setBit(slotIndex);
        m_size += _count;

        Byte* slotPtr = m_bufferBytes + slotIndex * sizeof(ValueType);
        return reinterpret_cast<ValueType*>(slotPtr);
    }

    /**
     * @brief Allocates memory for a given number of T objects at a specific index.
     *
     * @param _idx The index at which to allocate memory.
     * @param _count The number of T objects to allocate memory for.
     * @return Pointer to the allocated memory, or nullptr if allocation fails.
     *
     * @throws std::bad_alloc from the new operator if allocation fails.
     * @throws std::runtime_error if the index is out of bounds.
     */
    ValueType* allocateAt(size_t _idx, size_t _count)
        requires(Policy == PoolAllocatorPolicy::manual_indexing)
    {
        if (_idx + _count >= m_capacity) throw std::runtime_error("Index out of bounds.");

        size_t actualCount = 0;

        for (size_t i = 0; i < _count; ++i)
        {
            if (m_slotsState.testBit(_idx + i)) return nullptr;
            m_slotsState.setBit(_idx + i);
            ++actualCount;
        }

        m_size += actualCount;

        Byte* slotPtr = m_bufferBytes + _idx * sizeof(ValueType);
        return reinterpret_cast<ValueType*>(slotPtr);
    }

    /**
     * @brief Deallocates memory for a given number of T objects.
     *
     * @param _ptr Pointer to the memory to deallocate.
     * @param _count The number of T objects to deallocate.
     * @return void
     *
     * @throws std::runtime_error if the pointer does not match the expected deallocation pointer
     * when using the stack policy.
     */
    void deallocate(ValueType* _ptr, size_t _count) noexcept
    {
        if (!owns(_ptr) || _count == 0) return;

        Byte* bytePtr = reinterpret_cast<Byte*>(_ptr);

        size_t offsetInBytes = bytePtr - m_bufferBytes;
        if (offsetInBytes % sizeof(ValueType) != 0) return;

        size_t slotIndex = offsetInBytes / sizeof(ValueType);
        if (slotIndex + _count > m_capacity) return;

        size_t actualCount = 0;

        for (size_t i = 0; i < _count && slotIndex + i < m_capacity; ++i)
        {
            if (!m_slotsState.testBit(slotIndex + i)) continue;
            m_slotsState.clearBit(slotIndex + i);
            ++actualCount;
        }

        m_size -= actualCount;
    }

    /**
     * @brief Deallocates memory for a given number of T objects at a specific index.
     *
     * @param _idx The index at which to deallocate memory.
     * @param _count The number of T objects to deallocate.
     * @return void
     *
     * @throws std::runtime_error if the index is out of bounds.
     */
    void deallocateAt(size_t _idx, size_t _count)
        requires(Policy == PoolAllocatorPolicy::manual_indexing)
    {
        if (_idx >= m_capacity) throw std::runtime_error("Index out of bounds.");

        size_t actualCount = 0;

        for (size_t i = 0; i < _count && _idx + i < m_capacity; ++i)
        {
            if (!m_slotsState.testBit(_idx + i)) continue;
            m_slotsState.clearBit(_idx + i);
            ++actualCount;
        }

        m_size -= actualCount;
    }

    /**
     * @brief Constructs an object of type U at the given pointer.
     *
     * @tparam Args Types of arguments to forward to the constructor of U.
     * @param _ptr Pointer to the object to construct.
     * @param _args Arguments to forward to the constructor.
     * @return void
     *
     * @throws noexcept If the constructor of U is nothrow constructible.
     */
    template <typename... Args>
    void construct(T* _ptr, Args&&... _args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if (!owns(_ptr)) return;

        ::new (static_cast<void*>(_ptr)) T(std::forward<Args>(_args)...);
    }

    /**
     * @brief Destroys an object of type T at the given pointer.
     *
     * @param _ptr Pointer to the object to destroy.
     * @return void
     *
     * @throws noexcept If the destructor of T is nothrow destructible.
     */
    void destroy(T* _ptr) noexcept(std::is_nothrow_destructible_v<T>)
    {
        if constexpr (!TriviallyDestructible<T>)
        {
            if (_ptr) _ptr->~T();
        }
    }

    /**
     * @brief Gets a pointer to the T object at the given index.
     *
     * @param _idx The index of the T object to get.
     * @return Pointer to the T object, or nullptr if the index is out of bounds or does not own
     * the slot.
     */
    ValueType* at(size_t _idx) const
        requires(Policy == PoolAllocatorPolicy::manual_indexing)
    {
        if (_idx >= m_capacity) return nullptr;

        size_t offsetInBytes = _idx * sizeof(ValueType);
        Byte* slotPtr = m_bufferBytes + offsetInBytes;

        if (!owns(slotPtr)) return nullptr;

        return reinterpret_cast<ValueType*>(slotPtr);
    }

    /**
     * @brief Defragments the pool allocator by moving allocated objects to the start of the buffer.
     *
     * @note This operation will invalidate all pointers to previously allocated objects.
     * @return void
     */
    void defragment()
    {
        if (m_size == 0 || m_size == m_capacity) return;

        size_t writeIndex = 0;

        for (size_t readIndex = 0; readIndex < m_capacity; ++readIndex)
        {
            if (!m_slotsState.testBit(readIndex)) continue;

            if (writeIndex != readIndex)
            {
                Byte* srcPtr = m_bufferBytes + readIndex * sizeof(ValueType);
                Byte* destPtr = m_bufferBytes + writeIndex * sizeof(ValueType);

                std::memcpy(destPtr, srcPtr, sizeof(ValueType));
            }

            m_slotsState.setBit(writeIndex);
            m_slotsState.clearBit(readIndex);

            ++writeIndex;
        }

        m_size = writeIndex;
    }

    void reset()
    {
        m_slotsState.clearAll();
        m_size = 0;
    }

    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

    [[nodiscard]] size_t used() const noexcept { return m_size; }

    [[nodiscard]] size_t available() const noexcept { return m_capacity - m_size; }

    [[nodiscard]] bool owns(void* _ptr) const noexcept
    {
        if (!_ptr || !m_bufferBytes) return false;

        uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(_ptr);
        uintptr_t bufferStart = reinterpret_cast<uintptr_t>(m_bufferBytes);
        uintptr_t bufferEnd = bufferStart + m_capacity * sizeof(ValueType);

        if (ptrAddr < bufferStart || ptrAddr >= bufferEnd ||
            (ptrAddr - bufferStart) % sizeof(ValueType) != 0)
        {
            return false;
        }

        size_t slotIndex = (ptrAddr - bufferStart) / sizeof(ValueType);
        return m_slotsState.testBit(slotIndex);
    }

    [[nodiscard]] Byte* getBuffer() const noexcept { return m_bufferBytes; }

    [[nodiscard]] bool operator==(const PoolAllocator& _other) const noexcept
    {
        return this == &_other ||
               (m_bufferBytes == _other.m_bufferBytes && m_slotsState == _other.m_slotsState &&
                m_capacity == _other.m_capacity && m_size == _other.m_size);
    }

    [[nodiscard]] bool operator!=(const PoolAllocator& _other) const noexcept
    {
        return !(*this == _other);
    }

   private:
    ValueType* allocateContiguous(size_t _count)
    {
        for (size_t startSlot = 0; startSlot <= m_capacity - _count; ++startSlot)
        {
            if (hasContiguousFreeSlots(startSlot, _count))
            {
                for (size_t i = 0; i < _count; ++i) m_slotsState.setBit(startSlot + i);

                m_size += _count;

                Byte* blockPtr = m_bufferBytes + startSlot * sizeof(ValueType);
                return reinterpret_cast<ValueType*>(blockPtr);
            }
        }

        return nullptr;
    }

    bool hasContiguousFreeSlots(size_t _startSlot, size_t _count) const noexcept
    {
        for (size_t i = 0; i < _count; ++i)
        {
            if (_startSlot + i >= m_capacity || m_slotsState.testBit(_startSlot + i)) return false;
        }

        return true;
    }
};

/**
 * @brief Type alias for a pool allocator with automatic indexing.
 */
template <typename T>
using AutomaticIndexingPoolAllocator = PoolAllocator<T, PoolAllocatorPolicy::automatic_indexing>;

/**
 * @brief Type alias for a pool allocator with manual indexing.
 */
template <typename T>
using ManualIndexingPoolAllocator = PoolAllocator<T, PoolAllocatorPolicy::manual_indexing>;

} // namespace pieces
