#pragma once

#include <type_traits>

/**
 * @brief Macro to define bitwise operators for enum classes.
 *
 * This macro overloads bitwise OR, AND, XOR, NOT, and their
 * assignment counterparts for a given enum class type.
 *
 * @param ENUMTYPE The enum class type to define the operators for.
 */
#define SATURN_DEFINE_ENUM_FLAGS_OPERATORS(ENUMTYPE)                                               \
                                                                                                   \
    [[nodiscard]] constexpr ENUMTYPE operator|(ENUMTYPE lhs, ENUMTYPE rhs)                         \
    {                                                                                              \
        using Underlying = std::underlying_type_t<ENUMTYPE>;                                       \
        return static_cast<ENUMTYPE>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs)); \
    }                                                                                              \
                                                                                                   \
    [[nodiscard]] constexpr ENUMTYPE operator&(ENUMTYPE lhs, ENUMTYPE rhs)                         \
    {                                                                                              \
        using Underlying = std::underlying_type_t<ENUMTYPE>;                                       \
        return static_cast<ENUMTYPE>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)); \
    }                                                                                              \
                                                                                                   \
    [[nodiscard]] constexpr ENUMTYPE operator^(ENUMTYPE lhs, ENUMTYPE rhs)                         \
    {                                                                                              \
        using Underlying = std::underlying_type_t<ENUMTYPE>;                                       \
        return static_cast<ENUMTYPE>(static_cast<Underlying>(lhs) ^ static_cast<Underlying>(rhs)); \
    }                                                                                              \
                                                                                                   \
    [[nodiscard]] constexpr ENUMTYPE operator~(ENUMTYPE lhs)                                       \
    {                                                                                              \
        using Underlying = std::underlying_type_t<ENUMTYPE>;                                       \
        return static_cast<ENUMTYPE>(~static_cast<Underlying>(lhs));                               \
    }                                                                                              \
                                                                                                   \
    inline ENUMTYPE& operator|=(ENUMTYPE& lhs, ENUMTYPE rhs)                                       \
    {                                                                                              \
        lhs = lhs | rhs;                                                                           \
        return lhs;                                                                                \
    }                                                                                              \
                                                                                                   \
    inline ENUMTYPE& operator&=(ENUMTYPE& lhs, ENUMTYPE rhs)                                       \
    {                                                                                              \
        lhs = lhs & rhs;                                                                           \
        return lhs;                                                                                \
    }                                                                                              \
                                                                                                   \
    inline ENUMTYPE& operator^=(ENUMTYPE& lhs, ENUMTYPE rhs)                                       \
    {                                                                                              \
        lhs = lhs ^ rhs;                                                                           \
        return lhs;                                                                                \
    }

namespace saturn
{
namespace utils
{

template <typename E>
[[nodiscard]] constexpr bool hasFlag(E _flags, E _flag) noexcept
{
    using Underlying = std::underlying_type_t<E>;
    return (static_cast<Underlying>(_flags) & static_cast<Underlying>(_flag)) ==
           static_cast<Underlying>(_flag);
}

} // namespace utils
} // namespace saturn
