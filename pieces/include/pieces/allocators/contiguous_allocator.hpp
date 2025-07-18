#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>

#include "pieces/templates.hpp"

namespace pieces
{

/**
 * @brief An enum class to define the policy for contiguous allocators.
 *
 * @tparam T The type of objects to allocate memory for.
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
 */
template <typename T, ContiguousAllocatorPolicy Policy = ContiguousAllocatorPolicy::linear>
    requires std::is_trivially_destructible_v<T>
class ContiguousAllocatorBase final : public NonCopyable
{
   public:
    using Byte = uint8_t;
    using ValueType = T;

   private:
    // Expressed in bytes
    Byte* m_bufferBytes;
    size_t m_offsetInBytes;

    // Expressed in Ts
    size_t m_capacity;
    size_t m_size;

   public:
    /**
     * @brief Constructs a ContiguousAllocatorBase with a given capacity.
     *
     * @param _capacity The number of T objects the allocator can hold.
     *
     * @throws std::invalid_argument if _capacity is zero.
     */
    explicit ContiguousAllocatorBase(size_t _capacity)
        : m_bufferBytes(nullptr), m_offsetInBytes(0), m_capacity(_capacity), m_size(0)
    {
        if (_capacity == 0) throw std::invalid_argument("Size must be greater than zero.");

        size_t bytesNeeded = _capacity * sizeof(ValueType);

        m_bufferBytes =
            static_cast<Byte*>(::operator new(bytesNeeded, std::align_val_t{alignof(ValueType)}));
    }

    ~ContiguousAllocatorBase()
    {
        ::operator delete(m_bufferBytes, std::align_val_t{alignof(ValueType)});
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

    ContiguousAllocatorBase& operator=(ContiguousAllocatorBase&& _other) noexcept
    {
        if (this == &_other) return *this;

        ::operator delete(m_bufferBytes, std::align_val_t{alignof(ValueType)});

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
    ValueType* allocate(size_t _count)
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

        size_t bytesNeeded = _count * sizeof(T);
        size_t capacityInBytes = m_capacity * sizeof(T);

        size_t remainingBytes = capacityInBytes - m_offsetInBytes;
        void* rawPtr = m_bufferBytes + m_offsetInBytes;
        void* alignedPtr = rawPtr;

        if (!std::align(alignof(T), bytesNeeded, alignedPtr, remainingBytes)) return nullptr;

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
        if constexpr (Policy == ContiguousAllocatorPolicy::stack)
        {
            if (!owns(_ptr) || _count == 0) return;

            size_t bytesToFree = _count * sizeof(ValueType);
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
     * @brief Constructs an object of type U at the given pointer.
     *
     * @tparam U The type of the object to construct.
     * @param _ptr Pointer to the object to construct.
     * @param _args Arguments to forward to the constructor.
     * @return void
     *
     * @throws noexcept If the constructor of U is nothrow constructible.
     */
    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
    {
        if (!owns(_ptr)) return;

        ::new (static_cast<void*>(_ptr)) U(std::forward<Args>(_args)...);
    }

    /**
     * @brief Destroys an object of type U at the given pointer.
     *
     * @tparam U The type of the object to destroy.
     * @param _ptr Pointer to the object to destroy.
     * @return void
     *
     * @throws noexcept If the destructor of U is nothrow destructible.
     */
    template <typename U>
    void destroy(U* _ptr) noexcept(std::is_nothrow_destructible_v<U>)
    {
        if (owns(_ptr)) _ptr->~U();
    }

    // Resets the allocator, clearing all allocated memory and resetting the size.
    void reset()
    {
        m_size = 0;
        m_offsetInBytes = 0;
    }

    // Returns the total capacity of the allocator in T slots.
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

    // Returns the number of T slots used in the allocator.
    [[nodiscard]] size_t used() const noexcept { return m_size; }

    // Returns the number of T slots available for allocation.
    [[nodiscard]] size_t available() const noexcept { return m_capacity - m_size; }

    // Checks if the allocator owns the given pointer.
    [[nodiscard]] bool owns(void* _ptr) const noexcept
    {
        if (!_ptr) return false;

        Byte* charPtr = reinterpret_cast<Byte*>(_ptr);

        if (charPtr < m_bufferBytes) return false;

        size_t ptrOffsetInBytes = charPtr - m_bufferBytes;
        return ptrOffsetInBytes < m_offsetInBytes;
    }

    // Returns the raw buffer pointer.
    [[nodiscard]] inline Byte* buffer() const noexcept { return m_bufferBytes; }

    bool operator==(const ContiguousAllocatorBase& other) const noexcept
    {
        return this == &other || m_bufferBytes == other.m_bufferBytes &&
                                     m_capacity == other.m_capacity &&
                                     m_offsetInBytes == other.m_offsetInBytes;
    }

    bool operator!=(const ContiguousAllocatorBase& other) const noexcept
    {
        return !(*this == other);
    }
};

/**
 * @brief A linear allocator that allocates memory in a contiguous block without deallocation.
 */
template <typename T>
using LinearAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::linear>;

/**
 * @brief A stack allocator that allocates memory in a stack-like manner, allowing deallocation
 * only in reverse order.
 */
template <typename T>
using StackAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::stack>;

/**
 * @brief A circular allocator that allocates memory in a circular buffer manner, allowing
 * wrap-around when the buffer is full.
 */
template <typename T>
using CircularAllocator = ContiguousAllocatorBase<T, ContiguousAllocatorPolicy::circular>;

} // namespace pieces
