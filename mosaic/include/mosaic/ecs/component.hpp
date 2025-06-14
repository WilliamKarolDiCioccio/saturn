#pragma once

#include <bitset>
#include <type_traits>

namespace mosaic
{
namespace ecs
{

constexpr size_t MAX_COMPONENTS = 64;

using ComponentID = uint32_t;

struct ComponentMask
{
    std::bitset<MAX_COMPONENTS> bits;

    ComponentMask() = default;

    ComponentMask set(ComponentID id) const
    {
        ComponentMask m = *this;
        m.bits.set(id);
        return m;
    }

    bool operator==(ComponentMask const &o) const { return bits == o.bits; }
};

template <typename T>
concept IsComponent = std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;

namespace detail
{

inline ComponentID nextComponentID = 0;

} // namespace detail

template <typename T>
struct Component
{
    static const ComponentID id;
};

template <typename T>
const ComponentID Component<T>::id = detail::nextComponentID++;

} // namespace ecs
} // namespace mosaic

namespace std
{

template <>
struct hash<mosaic::ecs::ComponentMask>
{
    size_t operator()(mosaic::ecs::ComponentMask const &m) const noexcept
    {
        return m.bits.to_ullong();
    }
};

} // namespace std
