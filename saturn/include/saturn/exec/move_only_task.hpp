#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace saturn
{
namespace exec
{

/**
 * @brief Move-only type-erased callable wrapper.
 *
 * A custom implementation of std::move_only_function for platforms where it is unavailable
 * (e.g., Android NDK). Provides identical semantics: move-only ownership, type-erased storage,
 * and small buffer optimization to avoid heap allocation for small callables.
 *
 * @tparam Signature Function signature (e.g., void(), int(float, double))
 *
 * @example
 *   MoveOnlyTask<void()> task = []() { doWork(); };
 *   if (task) task();
 *
 *   MoveOnlyTask<int(int, int)> add = [](int a, int b) { return a + b; };
 *   int result = add(2, 3); // result == 5
 */
template <typename Signature>
class MoveOnlyTask;

/**
 * @brief Partial specialization for function signatures.
 *
 * @tparam R Return type
 * @tparam Args Argument types
 */
template <typename R, typename... Args>
class MoveOnlyTask<R(Args...)>
{
   private:
    /// Small buffer size for SBO. Chosen to fit most lambdas with 1-2 captures.
    /// Aligned to pointer size for optimal access.
    static constexpr std::size_t k_sboSize = sizeof(void*) * 4; // 32 bytes on 64-bit
    static constexpr std::size_t k_sboAlign = alignof(std::max_align_t);

    /// Type-erased operation vtable
    struct VTable
    {
        void (*destroy)(void* storage) noexcept;
        void (*move)(void* dst, void* src) noexcept;
        R (*invoke)(void* storage, Args... args);
    };

    /// Generate vtable for a specific callable type stored inline (SBO)
    template <typename F>
    static constexpr VTable makeInlineVTable() noexcept
    {
        return VTable{
            // destroy
            [](void* storage) noexcept { std::launder(reinterpret_cast<F*>(storage))->~F(); },
            // move
            [](void* dst, void* src) noexcept
            {
                new (dst) F(std::move(*std::launder(reinterpret_cast<F*>(src))));
                std::launder(reinterpret_cast<F*>(src))->~F();
            },
            // invoke
            [](void* storage, Args... args) -> R
            {
                return std::invoke(*std::launder(reinterpret_cast<F*>(storage)),
                                   std::forward<Args>(args)...);
            },
        };
    }

    /// Generate vtable for a specific callable type stored on heap
    template <typename F>
    static constexpr VTable makeHeapVTable() noexcept
    {
        return VTable{
            // destroy
            [](void* storage) noexcept { delete *reinterpret_cast<F**>(storage); },
            // move (just move the pointer)
            [](void* dst, void* src) noexcept
            {
                *reinterpret_cast<F**>(dst) = *reinterpret_cast<F**>(src);
                *reinterpret_cast<F**>(src) = nullptr;
            },
            // invoke
            [](void* storage, Args... args) -> R
            { return std::invoke(**reinterpret_cast<F**>(storage), std::forward<Args>(args)...); },
        };
    }

    /// Check if type F can be stored inline (SBO)
    template <typename F>
    static constexpr bool canStoreInline() noexcept
    {
        return sizeof(F) <= k_sboSize && alignof(F) <= k_sboAlign &&
               std::is_nothrow_move_constructible_v<F>;
    }

    /// Storage: either inline buffer or heap pointer
    alignas(k_sboAlign) unsigned char m_storage[k_sboSize];

    /// Vtable pointer (nullptr if empty)
    const VTable* m_vtable = nullptr;

   public:
    /// @brief Default constructor. Creates empty task.
    MoveOnlyTask() noexcept = default;

    /// @brief Nullptr constructor. Creates empty task.
    MoveOnlyTask(std::nullptr_t) noexcept {}

    /**
     * @brief Construct from callable.
     *
     * @tparam F Callable type (lambda, function pointer, functor)
     * @param f Callable to store
     *
     * Uses SBO if callable is small enough, otherwise heap-allocates.
     */
    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, MoveOnlyTask> &&
                                          std::is_invocable_r_v<R, std::decay_t<F>, Args...>>>
    MoveOnlyTask(F&& f)
    {
        using DecayedF = std::decay_t<F>;

        if constexpr (canStoreInline<DecayedF>())
        {
            // Store inline (SBO)
            new (m_storage) DecayedF(std::forward<F>(f));
            static constexpr VTable vtable = makeInlineVTable<DecayedF>();
            m_vtable = &vtable;
        }
        else
        {
            // Heap allocate
            *reinterpret_cast<DecayedF**>(m_storage) = new DecayedF(std::forward<F>(f));
            static constexpr VTable vtable = makeHeapVTable<DecayedF>();
            m_vtable = &vtable;
        }
    }

    /// @brief Destructor. Destroys stored callable.
    ~MoveOnlyTask()
    {
        if (m_vtable)
        {
            m_vtable->destroy(m_storage);
        }
    }

    // Non-copyable
    MoveOnlyTask(const MoveOnlyTask&) = delete;
    MoveOnlyTask& operator=(const MoveOnlyTask&) = delete;

    /// @brief Move constructor.
    MoveOnlyTask(MoveOnlyTask&& other) noexcept : m_vtable(other.m_vtable)
    {
        if (m_vtable)
        {
            m_vtable->move(m_storage, other.m_storage);
            other.m_vtable = nullptr;
        }
    }

    /// @brief Move assignment.
    MoveOnlyTask& operator=(MoveOnlyTask&& other) noexcept
    {
        if (this != &other)
        {
            // Destroy current
            if (m_vtable)
            {
                m_vtable->destroy(m_storage);
            }

            // Move from other
            m_vtable = other.m_vtable;
            if (m_vtable)
            {
                m_vtable->move(m_storage, other.m_storage);
                other.m_vtable = nullptr;
            }
        }
        return *this;
    }

    /// @brief Nullptr assignment. Clears the task.
    MoveOnlyTask& operator=(std::nullptr_t) noexcept
    {
        if (m_vtable)
        {
            m_vtable->destroy(m_storage);
            m_vtable = nullptr;
        }
        return *this;
    }

    /**
     * @brief Invoke the stored callable.
     *
     * @param args Arguments to forward to callable
     * @return Result of invocation
     *
     * @note Behavior is undefined if the task is empty.
     */
    R operator()(Args... args) { return m_vtable->invoke(m_storage, std::forward<Args>(args)...); }

    /**
     * @brief Check if task contains a callable.
     *
     * @return true if callable is stored, false if empty
     */
    explicit operator bool() const noexcept { return m_vtable != nullptr; }

    /**
     * @brief Swap with another task.
     *
     * @param other Task to swap with
     */
    void swap(MoveOnlyTask& other) noexcept
    {
        MoveOnlyTask tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }
};

/// @brief Swap two MoveOnlyTask objects.
template <typename R, typename... Args>
void swap(MoveOnlyTask<R(Args...)>& lhs, MoveOnlyTask<R(Args...)>& rhs) noexcept
{
    lhs.swap(rhs);
}

/// @brief Compare MoveOnlyTask with nullptr.
template <typename R, typename... Args>
bool operator==(const MoveOnlyTask<R(Args...)>& task, std::nullptr_t) noexcept
{
    return !task;
}

/// @brief Compare nullptr with MoveOnlyTask.
template <typename R, typename... Args>
bool operator==(std::nullptr_t, const MoveOnlyTask<R(Args...)>& task) noexcept
{
    return !task;
}

/// @brief Compare MoveOnlyTask with nullptr (inequality).
template <typename R, typename... Args>
bool operator!=(const MoveOnlyTask<R(Args...)>& task, std::nullptr_t) noexcept
{
    return static_cast<bool>(task);
}

/// @brief Compare nullptr with MoveOnlyTask (inequality).
template <typename R, typename... Args>
bool operator!=(std::nullptr_t, const MoveOnlyTask<R(Args...)>& task) noexcept
{
    return static_cast<bool>(task);
}

} // namespace exec
} // namespace saturn
