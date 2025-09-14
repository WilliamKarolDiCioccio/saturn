#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include "entity.hpp"
#include "component.hpp"
#include "component_registry.hpp"

namespace mosaic
{
namespace ecs
{

template <typename... Ts>
[[nodiscard]] static inline bool areComponentsRegistered(const ComponentRegistry* _registry)
{
    return (_registry->isRegistered<Ts>() && ...);
}

template <typename... Ts>
[[nodiscard]] static inline constexpr ComponentSignature getSignatureFromTypes(
    const ComponentRegistry* _registry)
{
    ComponentSignature sig(_registry->maxCount());
    (sig.setBit(_registry->getID<Ts>()), ...);
    return sig;
}

template <typename... Ts>
[[nodiscard]] static inline constexpr size_t getStrideSizeInBytes()
{
    return sizeof(EntityMeta) + (sizeof(Ts) + ...);
}

[[nodiscard]] static inline std::unordered_map<ComponentID, size_t>
getComponentOffsetsInBytesFromSignature(const ComponentRegistry* _registry,
                                        ComponentSignature _signature)
{
    std::vector<std::pair<ComponentID, size_t>> idSizePairs;
    for (ComponentID id = 0; id < _registry->maxCount(); ++id)
    {
        if (_signature.testBit(id)) idSizePairs.emplace_back(id, _registry->info(id).size);
    }

    std::sort(idSizePairs.begin(), idSizePairs.end());

    std::unordered_map<ComponentID, size_t> componentOffsets;
    size_t currentOffset = sizeof(EntityMeta);

    for (const auto& [id, size] : idSizePairs)
    {
        componentOffsets[id] = currentOffset;
        currentOffset += size;
    }

    return componentOffsets;
}

} // namespace ecs
} // namespace mosaic
