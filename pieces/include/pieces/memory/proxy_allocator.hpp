#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <functional>

#include "pieces/core/templates.hpp"

namespace pieces
{

/**
 * @brief A wrapper around an allocator that allows to instruement it.
 */
template <typename T, typename A = BaseAllocator<T>>
    requires std::is_trivially_destructible_v<T>
class ProxyAllocator final : public NonCopyable<ProxyAllocator<T, A>>,
                             NonMovable<ProxyAllocator<T, A>>
{
   public:
    using Byte = A::Byte;
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
    explicit ProxyAllocator(A* _allocator, const OnAllocateCallback& _onAllocate,
                            const OnDeallocateCallback& _onDeallocate,
                            const OnConstructCallback& _onConstruct,
                            const OnDestroyCallback& _onDestroy)
        : m_allocator(_allocator),
          m_onAllocate(_onAllocate),
          m_onDeallocate(_onDeallocate),
          m_onConstruct(_onConstruct),
          m_onDestroy(_onDestroy)
    {
        if (!_allocator) throw std::invalid_argument("Allocator cannot be null.");
    }

   public:
    [[nodiscard]] T* allocate(size_t _count)
    {
        T* ptr = m_allocator.allocate(_count);
        if (ptr) m_onAllocate(ptr, _count);
        return ptr;
    }

    void deallocate(T* _ptr, size_t _count)
    {
        if (_ptr) m_onDeallocate(static_cast<void*>(_ptr), _count);
        m_allocator.deallocate(static_cast<T*>(_ptr), _count);
    }

    template <typename U, typename... Args>
    void construct(U* _ptr, Args&&... _args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
    {
        if (_ptr) m_onConstruct(static_cast<void*>(_ptr), std::forward<Args>(_args)...);
        m_allocator.construct(_ptr, std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept(std::is_nothrow_destructible_v<U>)
    {
        if (_ptr) m_onDestroy(static_cast<void*>(_ptr));
        m_allocator.destroy(_ptr);
    }

    [[nodiscard]] bool owns(void* _ptr) const noexcept { return m_allocator.owns(_ptr); }

    // Not instrumentable but required by the Allocator concept.
    [[nodiscard]] Byte* getBuffer() const noexcept { return m_allocator.getBuffer(); }
};

} // namespace pieces
