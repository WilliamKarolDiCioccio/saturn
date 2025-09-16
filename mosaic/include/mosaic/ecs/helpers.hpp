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
[[nodiscard]] inline bool areComponentsRegistered(const ComponentRegistry* _registry)
{
    return (_registry->isRegistered<Ts>() && ...);
}

template <typename... Ts>
[[nodiscard]] inline constexpr ComponentSignature getSignatureFromTypes(
    const ComponentRegistry* _registry)
{
    ComponentSignature sig(_registry->maxCount());
    (sig.setBit(_registry->getID<Ts>()), ...);
    return sig;
}

[[nodiscard]] inline size_t calculateStrideFromSignature(const ComponentRegistry* _registry,
                                                         const ComponentSignature& sig)
{
    size_t stride = sizeof(EntityMeta);

    for (size_t i = 0; i < _registry->count(); ++i)
    {
        if (sig.testBit(i))
        {
            const auto& info = _registry->info(i);

            // This ensures proper alignment for each component
            stride = (stride + info.alignment - 1) & ~(info.alignment - 1);
            stride += info.size;
        }
    }

    return stride;
}

[[nodiscard]] inline std::unordered_map<ComponentID, size_t>
getComponentOffsetsInBytesFromSignature(const ComponentRegistry* _registry,
                                        const ComponentSignature& _signature)
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
        const auto& info = _registry->info(id);

        // This ensures proper alignment for each component
        currentOffset = (currentOffset + info.alignment - 1) & ~(info.alignment - 1);

        componentOffsets[id] = currentOffset;
        currentOffset += info.size;
    }

    return componentOffsets;
}

} // namespace ecs
} // namespace mosaic
