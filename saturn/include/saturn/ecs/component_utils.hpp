#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "entity.hpp"
#include "component.hpp"
#include "component_registry.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief Checks if all specified component types are registered in the component registry.
 *
 * @tparam Ts The component types to check.
 * @param _registry The component registry to check against.
 * @return true if all component types are registered, false otherwise.
 */
template <typename... Ts>
[[nodiscard]] inline bool areComponentsRegistered(const ComponentRegistry* _registry)
{
    return (_registry->isRegistered<Ts>() && ...);
}

/**
 * @brief Generates a component signature from the specified component types.
 *
 * @tparam Ts The component types to include in the signature.
 * @param _registry The component registry for component type information.
 * @return A ComponentSignature representing the combination of the specified component types.
 */
template <typename... Ts>
[[nodiscard]] inline constexpr ComponentSignature getSignatureFromTypes(
    const ComponentRegistry* _registry)
{
    ComponentSignature sig(_registry->maxCount());
    (sig.setBit(_registry->getID<Ts>()), ...);
    return sig;
}

/**
 * @brief Calculates the stride (in bytes) required for an archetype based on its component
 * signature.
 *
 * @param _registry The component registry for component type information.
 * @param sig The component signature defining the archetype.
 * @return The total stride size in bytes for the archetype.
 *
 * @note The stride includes the size of EntityMeta and accounts for alignment requirements of each
 * component.
 */
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

/**
 * @brief Computes the byte offsets of each component in an archetype based on its signature.
 *
 * @param _registry The component registry for component type information.
 * @param _signature The component signature defining the archetype.
 * @return A map of ComponentID to their respective byte offsets within the archetype's data rows.
 */
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
} // namespace saturn
