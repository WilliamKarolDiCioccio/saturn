#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <cstring>

#include "pieces/core/templates.hpp"

namespace pieces
{

/**
 * @brief An enum class to define the policy for contiguous allocators.
 */
enum class ContiguousAllocatorPolicy : uint8_t
{
    linear,
    stack,
    circular,
};

/**
 * @brief A templetized base class for contiguous memory allocators (linear, stack, circular).
 *
 * The three policies are:
 *
 * - linear: Allocates memory linearly, does not allow deallocation.
 *
 * - stack: Allocates memory in a stack-like manner, allowing deallocation only in reverse order.
 *
 * - circular: Allocates memory in a circular buffer manner, allowing wrap-around when the buffer is
 * full.
 *
 * @tparam T The type of objects to allocate memory for.
 * @tparam Policy The policy for the allocator (default is linear).
 *
 * @note This allocator operates in base T for size, alignment, and all related operations.
 * @note Due to the performance-critical nature of this allocator, it does not support
 * non-trivially-destructible types.
 */
template <typename T, ContiguousAllocatorPolicy Policy = ContiguousAllocatorPolicy::linear>
class ContiguousAllocatorBase final : public NonCopyable<ContiguousAllocatorBase<T, Policy>>
{
   public:
    using Byte = uint8_t;
    using ValueType = T;

   private:
    static constexpr size_t ALIGNOF_VALUE = alignof(ValueType);
    static constexpr size_t SIZEOF_VALUE = sizeof(ValueType);

    // Expressed in bytes
    Byte* m_bufferBytes;
    size_t m_offsetInBytes;

    // Expressed in Ts
    size_t m_capacity;
    size_t m_size;

   public:
    /**
     * @brief Constructs a ContiguousAllocatorBase with a given capacity in T objects.
     *
     * @param _capacity The number of T objects the allocator can hold.
     *
     * @throws std::invalid_argument if _capacity is zero.
     */
    explicit ContiguousAllocatorBase(size_t _capacity)
        : m_bufferBytes(nullptr), m_offsetInBytes(0), m_capacity(_capacity), m_size(0)
    {
        if (_capacity == 0) throw std::invalid_argument("Size must be greater than zero.");

        size_t bytesNeeded = _capacity * SIZEOF_VALUE;

        m_bufferBytes =
            static_cast<Byte*>(::operator new(bytesNeeded, std::align_val_t{ALIGNOF_VALUE}));

        std::memset(m_bufferBytes, 0, bytesNeeded);
    }

    ~ContiguousAllocatorBase()
    {
        ::operator delete(m_bufferBytes, std::align_val_t{ALIGNOF_VALUE});
    }

    ContiguousAllocatorBase(ContiguousAllocatorBase&& _other) noexcept
        : m_bufferBytes(_other.m_bufferBytes),
          m_offsetInBytes(_other.m_offsetInBytes),
          m_capacity(_other.m_capacity),
          m_size(_other.m_size)
    {
        _other.m_bufferBytes = nullptr;
        _other.m_offsetInBytes = 0;
        _other.m_size = 0;
    }

    [[nodiscard]] ContiguousAllocatorBase& operator=(ContiguousAllocatorBase&& _other) noexcept
    {
        if (this == &_other) return *this;

        ::operator delete(m_bufferBytes, std::align_val_t{ALIGNOF_VALUE});

        m_bufferBytes = _other.m_bufferBytes;
        m_offsetInBytes = _other.m_offsetInBytes;
        m_capacity = _other.m_capacity;
        m_size = _other.m_size;

        _other.m_bufferBytes = nullptr;
        _other.m_offsetInBytes = 0;
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
    [[nodiscard]] ValueType* allocate(size_t _count)
    {
        if (_count == 0) return nullptr;

        if (_count > m_capacity - m_size)
        {
            if constexpr (Policy == ContiguousAllocatorPolicy::circular)
            {
                reset();
            }
            else
            {
                return nullptr;
            }
        }

        size_t bytesNeeded = _count * SIZEOF_VALUE;
        size_t capacityInBytes = m_capacity * SIZEOF_VALUE;

        size_t remainingBytes = capacityInBytes - m_offsetInBytes;
        void* rawPtr = m_bufferBytes + m_offsetInBytes;
        void* alignedPtr = rawPtr;

        if (!std::align(ALIGNOF_VALUE, bytesNeeded, alignedPtr, remainingBytes)) return nullptr;

        m_offsetInBytes = static_cast<Byte*>(alignedPtr) - m_bufferBytes + bytesNeeded;
        m_size += _count;

        return static_cast<T*>(alignedPtr);
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
    void deallocate([[maybe_unused]] ValueType* _ptr,
                    [[maybe_unused]] size_t _count) noexcept(Policy !=
                                                             ContiguousAllocatorPolicy::stack)
    {
        if (!owns(_ptr) || _count == 0) return;

        if constexpr (Policy == ContiguousAllocatorPolicy::stack)
        {
            size_t bytesToFree = _count * SIZEOF_VALUE;
            Byte* expectedPtr = m_bufferBytes + m_offsetInBytes - bytesToFree;

            if (reinterpret_cast<Byte*>(_ptr) != expectedPtr)
            {
                throw std::runtime_error("Unexpected pointer for deallocation.");
            }

            m_offsetInBytes -= bytesToFree;
        }

        m_size -= _count;
    }

    /**
     * @brief Constructs an object of type T at the given pointer.
     *
     * @tparam Args Types of arguments to forward to the constructor of T.
     * @param _ptr Pointer to the object to construct.
     * @param _args Arguments to forward to the constructor.
     * @return void
     *
     * @throws noexcept If the constructor of T is nothrow constructible.
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

    void reset()
    {
        m_size = 0;
        m_offsetInBytes = 0;
    }

    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

    [[nodiscard]] size_t used() const noexcept { return m_size; }

    [[nodiscard]] size_t available() const noexcept { return m_capacity - m_size; }

    [[nodiscard]] bool owns(void* _ptr) const noexcept
    {
        if (!_ptr) return false;

        Byte* bytePtr = reinterpret_cast<Byte*>(_ptr);

        if (bytePtr < m_bufferBytes) return false;

        size_t ptrOffsetInBytes = bytePtr - m_bufferBytes;
        return ptrOffsetInBytes < m_offsetInBytes;
    }

    [[nodiscard]] Byte* getBuffer() const noexcept { return m_bufferBytes; }

    [[nodiscard]] bool operator==(const ContiguousAllocatorBase& other) const noexcept
    {
        return this == &other ||
               (m_bufferBytes == other.m_bufferBytes && m_capacity == other.m_capacity &&
                m_offsetInBytes == other.m_offsetInBytes);
    }

    [[nodiscard]] bool operator!=(const ContiguousAllocatorBase& other) const noexcept
    {
        return !(*this == other);
    }
};

/**
 * @brief Type alias for a contiguous allocator with linear allocation policy.
 */
template <typename T>
using LinearAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::linear>;

/**
 * @brief Type alias for a stack allocator that allocates memory in a stack-like manner.
 */
template <typename T>
using StackAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::stack>;

/**
 * @brief Type alias for a circular allocator that allocates memory in a circular buffer manner.
 */
template <typename T>
using CircularAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::circular>;

} // namespace pieces
