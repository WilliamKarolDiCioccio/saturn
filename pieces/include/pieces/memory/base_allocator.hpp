#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>

#include "pieces/core/templates.hpp"

namespace pieces
{

/**
 * @brief A simple wrapper around new and delete.
 *
 * @tparam T The type of objects to allocate memory for.
 */
template <typename T>
class BaseAllocator final : public NonCopyable, NonMovable
{
   public:
    using Byte = uint8_t;
    using ValueType = T;

   public:
    BaseAllocator() = default;

   public:
    [[nodiscard]] T* allocate(size_t _count)
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
    void construct(T* _ptr, Args&&... _args)
    {
        if (!owns(_ptr)) throw std::invalid_argument("Pointer cannot be null.");

        ::new (static_cast<void*>(_ptr)) T(std::forward<Args>(_args)...);
    }

    template <typename U>
    void destroy(U* _ptr) noexcept
    {
        if constexpr (!TriviallyDestructible<U>)
        {
            if (_ptr) _ptr->~U();
        }
    }

    [[nodiscard]] bool owns(void* _ptr) const noexcept { return _ptr != nullptr; }

    bool operator==(const BaseAllocator& _other) const noexcept { return *this == _other; }
    bool operator!=(const BaseAllocator& _other) const noexcept { return !(*this == _other); };
};

} // namespace pieces
