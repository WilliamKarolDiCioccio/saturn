#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <functional>

#include "templates.hpp"

namespace pieces
{

/**
 * @brief Concept for a generic allocator.
 *
 * This concept checks if the type `A` meets the minimum requirements of an allocator, including
 * allocation, deallocation, construction and destruction.
 */
template <typename A>
concept Allocator = requires(A a, size_t n) {
    { a.allocate(n) } -> std::same_as<typename A::ValueType*>;
    { a.deallocate(nullptr, n) };
    { a.construct(std::declval<typename A::ValueType*>(), std::declval<typename A::ValueType>()) };
    { a.destroy(std::declval<typename A::ValueType*>()) };
    { a.owns(nullptr) } -> std::same_as<bool>;
    { a == a } -> std::same_as<bool>;
    { a != a } -> std::same_as<bool>;
};

/**
 * @brief A simple wrapper around new and delete.
 *
 * @tparam T The type of objects to allocate memory for.
 */
template <typename T>
    requires std::is_trivially_destructible_v<T>
class BaseAllocator final : public NonCopyable, NonMovable
{
   public:
    using ValueType = T;

   public:
    BaseAllocator() = default;

   public:
    T* allocate(size_t _count)
    {
        if (_count == 0) return nullptr;

        size_t bytesNeeded = _count * sizeof(T);
        T* ptr = static_cast<T*>(::operator new(bytesNeeded));

        return ptr;
    }

    void deallocate(T* _ptr, size_t _count) noexcept
    {
        if (owns(_ptr) && _count > 0) ::operator delete(_ptr);
    }

    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args)
    {
        if (!owns(_ptr)) throw std::invalid_argument("Pointer cannot be null.");
        ::new (static_cast<void*>(_ptr)) U(std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept
    {
        if (_ptr) _ptr->~U();
    }

    bool owns(void* _ptr) const noexcept { return _ptr != nullptr; }

    bool operator==(const BaseAllocator& _other) const noexcept { return *this == _other; }
    bool operator!=(const BaseAllocator& _other) const noexcept { return !(*this == _other); };
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

    void reset() { m_offsetInBytes = 0; }

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

/**
 * @brief A wrapper around an allocator that allows to instruement it.
 */
template <typename T, typename A = BaseAllocator<T>>
    requires std::is_trivially_destructible_v<T>
class ProxyAllocator final : public NonCopyable, NonMovable
{
   public:
    using ValueType = A::ValueType;

   private:
    using OnAllocateCallback = std::function<void(void*, size_t)>;
    using OnDeallocateCallback = std::function<void(void*, size_t)>;
    using OnConstructCallback = std::function<void(void*, const void*)>;
    using OnDestroyCallback = std::function<void(void*)>;

    OnAllocateCallback m_onAllocate;
    OnDeallocateCallback m_onDeallocate;
    OnConstructCallback m_onConstruct;
    OnDestroyCallback m_onDestroy;

    A* m_allocator;

   public:
    ProxyAllocator(A* _allocator, const OnAllocateCallback& _onAllocate,
                   const OnDeallocateCallback& _onDeallocate,
                   const OnConstructCallback& _onConstruct, const OnDestroyCallback& _onDestroy)
        : m_allocator(_allocator),
          m_onAllocate(_onAllocate),
          m_onDeallocate(_onDeallocate),
          m_onConstruct(_onConstruct),
          m_onDestroy(_onDestroy)
    {
        if (!_allocator) throw std::invalid_argument("Allocator cannot be null.");
    }

   public:
    T* allocate(size_t _count)
    {
        T* ptr = m_allocator.allocate(_count);
        if (ptr) m_onAllocate(ptr, _count);
        return ptr;
    }

    void deallocate(T* _ptr, size_t _count) noexcept
    {
        if (_ptr) m_onDeallocate(static_cast<void*>(_ptr), _count);
        m_allocator.deallocate(static_cast<T*>(_ptr), _count);
    }

    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args)
    {
        if (_ptr) m_onConstruct(static_cast<void*>(_ptr), std::forward<Args>(_args)...);
        m_allocator.construct(_ptr, std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept
    {
        if (_ptr) m_onDestroy(static_cast<void*>(_ptr));
        m_allocator.destroy(_ptr);
    }

    bool owns(void* _ptr) const noexcept { return m_allocator.owns(_ptr); }
};

} // namespace pieces
