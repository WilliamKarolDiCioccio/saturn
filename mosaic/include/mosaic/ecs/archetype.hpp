#pragma once

#include <bitset>
#include <unordered_map>

#include "component.hpp"
#include "typeless_sparse_set.hpp"

namespace mosaic
{
namespace ecs
{

/**
 * @brief The 'Archetype' class represents a collection of entities that share the same set of
 * components in an ECS architecture.
 *
 * Archetypes wrap the underlying storage mechanism (TypelessSparseSet). The main reason for this
 * wrapper is to associate a component signature with the storage and to keep track of component
 * offsets within the entity data therefore speeding up access all entity registry operations.
 */
class Archetype final
{
   private:
    using Byte = uint8_t;

    ComponentSignature m_signature;
    TypelessSparseSet<64, false> m_storage;
    std::unordered_map<ComponentID, size_t> m_componentOffsets;

   public:
    /**
     * @brief Constructs an Archetype with the given component signature, stride, and component
     * offsets.
     *
     * @param _signature The component signature representing the set of components in this
     * archetype.
     * @param _stride The size in bytes of each entity's data (including metadata and components).
     * @param _componentOffsets A mapping from component IDs to their byte offsets within the
     * entity data.
     */
    Archetype(ComponentSignature _signature, size_t _stride,
              std::unordered_map<ComponentID, size_t> _componentOffsets)
        : m_signature(_signature), m_storage(_stride), m_componentOffsets(_componentOffsets){};

   public:
    /**
     * @brief Inserts a new entity with the given ID and associated data into the archetype.
     *
     * @param _eid The ID of the entity to be inserted.
     * @param _data A pointer to the raw data representing the entity's metadata and components.
     */
    void insert(EntityID _eid, const Byte* _data) { m_storage.insert(_eid, _data); }

    /**
     * @brief Removes the entity with the specified ID from the archetype.
     *
     * @param _eid The ID of the entity to be removed.
     */
    void remove(EntityID _eid) { m_storage.remove(_eid); }

    /**
     * @brief Retrieves a pointer to the raw data of the entity with the given ID.
     *
     * @param _eid The ID of the entity whose data is to be retrieved.
     * @return A pointer to the raw data of the entity, or nullptr if the entity does not exist in
     * the archetype.
     */
    Byte* get(EntityID _eid) { return m_storage.get(_eid); }

    /**
     * @brief Retrieves the component signature of the archetype.
     *
     * @return const ComponentSignature&
     */
    [[nodiscard]] inline const ComponentSignature& signature() const noexcept
    {
        return m_signature;
    }

    /**
     * @brief Retrieves the mapping of component IDs to their byte offsets within the entity data.
     *
     * @return const std::unordered_map<ComponentID, size_t>&
     */
    [[nodiscard]] inline const std::unordered_map<ComponentID, size_t>& componentOffsets()
        const noexcept
    {
        return m_componentOffsets;
    }

    // Returns the size in bytes of each entity's data (metadata and components).
    [[nodiscard]] inline size_t stride() const noexcept { return m_storage.stride(); }

    // Returns the number of entities in the archetype.
    [[nodiscard]] inline size_t size() const noexcept { return m_storage.size(); }

    // Checks if the archetype is empty (contains no entities).
    [[nodiscard]] inline bool empty() const noexcept { return m_storage.empty(); }

    // Provides direct access to the underlying data storage.
    [[nodiscard]] inline Byte* data() noexcept { return m_storage.data().data(); }

    // Returns the memory usage of the archetype in bytes.
    [[nodiscard]] inline size_t memoryUsageInBytes() const noexcept
    {
        return m_storage.memoryUsageInBytes();
    }
};

} // namespace ecs
} // namespace mosaic
