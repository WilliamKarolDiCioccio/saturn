#pragma once

#include <concepts>
#include <type_traits>
#include <ranges>
#include <iterator>
#include <utility>
#include <string>
#include <string_view>

namespace pieces
{

// Type category concepts

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept SignedIntegral = Integral<T> && std::is_signed_v<T>;

template <typename T>
concept UnsignedIntegral = Integral<T> && !std::is_signed_v<T>;

template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Enum = std::is_enum_v<T>;

template <typename T>
concept Class = std::is_class_v<T>;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept TriviallyMovable = std::is_trivially_move_constructible_v<T>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept POD = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

// Pointer concepts

template <typename T>
concept Pointer = std::is_pointer_v<T>;

template <typename T>
concept SmartPointer = requires(T t) {
    { *t } -> std::same_as<typename T::element_type &>;
};

// String concepts

template <typename T>
concept StringLike = std::same_as<T, std::string> || std::same_as<T, std::string_view>;

// Callable concepts

template <typename F, typename... Args>
concept CallableWith = requires(F &&f, Args &&...args) {
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
};

template <typename F, typename R, typename... Args>
concept Returns = requires(F &&f, Args &&...args) {
    { std::invoke(std::forward<F>(f), std::forward<Args>(args)...) } -> std::same_as<R>;
};

// Utility concepts

template <typename T, typename U>
concept Same = std::same_as<T, U>;

template <typename T, typename U>
concept ConvertibleTo = std::convertible_to<T, U>;

template <typename Base, typename Derived>
concept DerivedFrom = std::derived_from<Derived, Base>;

template <typename T>
concept Movable = std::movable<T>;

template <typename T>
concept Copyable = std::copyable<T>;

template <typename T>
concept DefaultConstructible = std::default_initializable<T>;

template <typename T>
concept Destructible = std::is_destructible_v<T>;

/**
 * @brief Concept for a generic allocator.
 *
 * This concept checks if the type `A` meets the minimum requirements of an allocator, including
 * allocation, deallocation, construction and destruction.
 */
template <typename A>
concept Allocator = requires(A a, size_t n) {
    typename A::Byte;
    typename A::ValueType;
    { a.allocate(n) } -> std::same_as<typename A::ValueType *>;
    { a.deallocate(nullptr, n) };
    { a.construct(std::declval<typename A::ValueType *>(), std::declval<typename A::ValueType>()) };
    { a.destroy(std::declval<typename A::ValueType *>()) };
    { a.owns(nullptr) } -> std::same_as<bool>;
    { a == a } -> std::same_as<bool>;
    { a != a } -> std::same_as<bool>;
};

// Class templates for non-copyable and non-movable types

class NonCopyable
{
   protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;

   private:
    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(NonCopyable &&) = default;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};

class NonMovable
{
   protected:
    NonMovable() = default;
    virtual ~NonMovable() = default;

   private:
    NonMovable(const NonMovable &) = default;
    NonMovable &operator=(const NonMovable &) = default;
    NonMovable(NonMovable &&) = delete;
    NonMovable &operator=(NonMovable &&) = delete;
};

} // namespace pieces
