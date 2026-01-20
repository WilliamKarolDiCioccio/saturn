#pragma once

#include <functional>
#include <unordered_map>

#include <pieces/containers/bitset.hpp>

namespace saturn
{
namespace ecs
{

using ComponentID = size_t;
using ComponentSignature = pieces::BitSet;

struct ComponentMeta
{
    std::string name;
    size_t size;
    size_t alignment;
};

template <typename T>
concept Component = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T> &&
                    !std::is_pointer_v<T> && !std::is_reference_v<T> && !std::is_const_v<T> &&
                    !std::is_volatile_v<T> && std::is_standard_layout_v<T>;

namespace detail
{

template <typename... Ts>
struct TypeList
{
};

template <Component... Components>
using From = TypeList<Components...>;

template <Component... Components>
using Add = TypeList<Components...>;

template <Component... Components>
using Remove = TypeList<Components...>;

template <Component... Components>
using With = TypeList<Components...>;

} // namespace detail

} // namespace ecs
} // namespace saturn

namespace std
{

template <>
struct hash<pieces::BitSet>
{
    size_t operator()(const pieces::BitSet& _bitset) const noexcept
    {
        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr uint64_t FNV_PRIME = 1099511628211ULL;

        uint64_t hash = FNV_OFFSET_BASIS;

        const auto* words = _bitset.data();
        const size_t wordCount = _bitset.wordCount();

        for (size_t i = 0; i < wordCount; ++i)
        {
            if (words[i] != 0)
            {
                hash ^= words[i];
                hash *= FNV_PRIME;
            }
        }

        return static_cast<size_t>(hash);
    }
};

} // namespace std
