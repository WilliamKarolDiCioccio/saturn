#pragma once

#include <algorithm>
#include <optional>
#include <unordered_map>

#include "entity.hpp"
#include "component.hpp"
#include "archetype.hpp"
#include "component_registry.hpp"
#include "component_utils.hpp"
#include "entity_view.hpp"
#include "entity_allocation_helper.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief The 'EntityRegistry' class manages the lifecycle of entities and their associated
 components within an ECS architecture.
 *
 * It provides functionality to create, destroy, and modify entities, as well as to query
 * entities based on their components.
 *
 * This is a purely archetypal implementation, meaning that entities are grouped by their component
 * signatures into archetypes and tightly packed together in tuples of components alongside their
 * metadata. This design reflects common access patterns in ECS systems, optimizing for cache
 * locality and iteration performance. Also, the purely type-unaware design means that the
 * EntityRegistry does not need to be templated on component types, allowing for a more flexible
 * and dynamic system such as runtime component registration from scripts or plugins.
 */
class EntityRegistry final
{
   private:
    using Byte = uint8_t;

    std::unordered_map<ComponentSignature, std::unique_ptr<Archetype>> m_archetypes;
    std::unordered_map<EntityID, Archetype*> m_entityToArchetype;
    const ComponentRegistry* m_componentRegistry;
    EntityAllocationHelper m_EntityAllocationHelper;

   public:
    /**
     * @brief Constructs an EntityRegistry with the given component registry.
     *
     * @param _componentRegistry The component registry for component type information.
     */
    EntityRegistry(const ComponentRegistry* _componentRegistry)
        : m_componentRegistry(_componentRegistry) {};

   public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Single Entity API
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Creates a new entity with the specified components.
     *
     * @tparam Ts The component types to be added to the new entity.
     * @return The metadata of the newly created entity.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    EntityMeta createEntity()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        auto stride = calculateStrideFromSignature(m_componentRegistry, sig);

        EntityMeta meta = m_EntityAllocationHelper.getID();
        Archetype* arch = getOrCreateArchetype(sig, stride);

        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        std::vector<Byte> row(stride);
        Byte* rowPtr = row.data();

        new (rowPtr) EntityMeta{meta};

        ((new (rowPtr + componentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

        arch->insert(meta.id, rowPtr);
        m_entityToArchetype[meta.id] = arch;

        return meta;
    }

    /**
     * @brief Creates a new entity with the specified components, initialized with constructor
     * arguments.
     *
     * @tparam Ts The component types to be added to the new entity.
     * @tparam ArgTuples The tuple types containing constructor arguments for each component.
     * @param _argTuples Tuples of constructor arguments for each component (use std::make_tuple).
     * @return The metadata of the newly created entity.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts, typename... ArgTuples>
        requires(sizeof...(Ts) == sizeof...(ArgTuples))
    EntityMeta createEntity(ArgTuples&&... _argTuples)
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        auto stride = calculateStrideFromSignature(m_componentRegistry, sig);

        EntityMeta meta = m_EntityAllocationHelper.getID();
        Archetype* arch = getOrCreateArchetype(sig, stride);

        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        std::vector<Byte> row(stride);
        Byte* rowPtr = row.data();

        new (rowPtr) EntityMeta{meta};

        // Construct each component with its args via tuple unpacking
        size_t idx = 0;
        (
            [&]<typename T, typename ArgTuple>(ArgTuple&& argTuple)
            {
                auto offset = componentOffsets[m_componentRegistry->getID<T>()];
                std::apply([&](auto&&... args)
                           { new (rowPtr + offset) T(std::forward<decltype(args)>(args)...); },
                           std::forward<ArgTuple>(argTuple));
            }.template operator()<Ts>(std::forward<ArgTuples>(_argTuples)),
            ...);

        arch->insert(meta.id, rowPtr);
        m_entityToArchetype[meta.id] = arch;

        return meta;
    }

    /**
     * @brief Destroys the entity with the specified ID, removing it from its archetype and freeing
     * its ID.
     *
     * @param _eid The ID of the entity to be destroyed.
     */
    void destroyEntity(EntityID _eid)
    {
        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return;

        Archetype* arch = m_entityToArchetype.at(_eid);

        arch->remove(_eid);
        m_entityToArchetype.erase(_eid);
        m_EntityAllocationHelper.freeID(_eid);
    }

    /**
     * @brief Modifies the components of the entity with the specified ID by adding and/or removing
     * specified components.
     *
     * If the entity already has some of the specified components, they will not be duplicated.
     * If the entity does not have some of the components to be removed, they will be ignored.
     * The entity will be moved to the corresponding archetype that matches its updated component
     * signature.
     *
     * @tparam AddComponents The component types to be added to the entity.
     * @tparam RemoveComponents The component types to be removed from the entity.
     * @param _eid The ID of the entity whose components are to be modified.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... AddComponents, Component... RemoveComponents>
    void modifyComponents(EntityID _eid, detail::Add<AddComponents...>,
                          detail::Remove<RemoveComponents...>)
    {
        // validate registrations (guard empty packs)
        if ((sizeof...(AddComponents) > 0 &&
             !areComponentsRegistered<AddComponents...>(m_componentRegistry)) ||
            (sizeof...(RemoveComponents) > 0 &&
             !areComponentsRegistered<RemoveComponents...>(m_componentRegistry)))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto mapIt = m_entityToArchetype.find(_eid);
        if (mapIt == m_entityToArchetype.end()) return; // entity not found (no-op)

        Archetype* oldArch = mapIt->second;
        Byte* oldRowPtr = oldArch->get(_eid);
        if (!oldRowPtr) return; // defensive

        ComponentSignature oldSig = oldArch->signature();

        // compute destination signature
        ComponentSignature newSig = oldSig;
        (newSig.setBit(m_componentRegistry->getID<AddComponents>()), ...);
        (newSig.clearBit(m_componentRegistry->getID<RemoveComponents>()), ...);

        // no-op if signature unchanged
        if (oldSig == newSig) return;

        size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
        Archetype* newArch = getOrCreateArchetype(newSig, newStride);

        if (newArch == oldArch) return; // safety (shouldn't happen, but cheap guard)

        std::vector<Byte> newRow(newStride);
        Byte* newRowPtr = newRow.data();

        // copy EntityMeta
        EntityMeta meta = *reinterpret_cast<EntityMeta*>(oldRowPtr);
        new (newRowPtr) EntityMeta{meta};

        // copy surviving components from old row to the corresponding new offsets
        auto oldOffsets = oldArch->componentOffsets(); // map<CompID, offset>
        auto newOffsets = newArch->componentOffsets(); // map<CompID, offset>

        for (const auto& [compID, srcOffset] : oldOffsets)
        {
            auto destIt = newOffsets.find(compID);
            if (destIt == newOffsets.end()) continue; // component removed
            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + destIt->second, oldRowPtr + srcOffset, compSize);
        }

        // default-construct newly added components (if any)
        if constexpr (sizeof...(AddComponents) > 0)
        {
            ((new (newRowPtr + newOffsets[m_componentRegistry->getID<AddComponents>()])
                  AddComponents()),
             ...);
        }

        // perform the migration: insert into destination archetype, remove from source, update
        // mapping
        newArch->insert(meta.id, newRowPtr);
        oldArch->remove(meta.id);
        m_entityToArchetype[meta.id] = newArch;
    }

    /**
     * @brief Adds the specified components to the entity with the given ID.
     *
     * @tparam Ts The component types to add.
     * @param _eid The ID of the entity to which components will be added.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    void addComponents(EntityID _eid)
    {
        modifyComponents(_eid, detail::Add<Ts...>{}, detail::Remove<>{});
    }

    /**
     * @brief Removes the specified components from the entity with the given ID.
     *
     * @tparam Ts The component types to remove.
     * @param _eid The ID of the entity from which components will be removed.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    void removeComponents(EntityID _eid)
    {
        modifyComponents(_eid, detail::Add<>{}, detail::Remove<Ts...>{});
    }

    /**
     * @brief Modifies the components of the entity with the specified ID by adding and/or removing
     * specified components, with constructor arguments for added components.
     *
     * If the entity already has some of the specified components, they will not be duplicated.
     * If the entity does not have some of the components to be removed, they will be ignored.
     * The entity will be moved to the corresponding archetype that matches its updated component
     * signature.
     *
     * @tparam AddComponents The component types to be added to the entity.
     * @tparam RemoveComponents The component types to be removed from the entity.
     * @tparam ArgTuples The tuple types containing constructor arguments for added components.
     * @param _eid The ID of the entity whose components are to be modified.
     * @param _addTag Tag for added components.
     * @param _removeTag Tag for removed components.
     * @param _argTuples Tuples of constructor arguments for each added component.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... AddComponents, Component... RemoveComponents, typename... ArgTuples>
        requires(sizeof...(AddComponents) == sizeof...(ArgTuples))
    void modifyComponents(EntityID _eid, detail::Add<AddComponents...>,
                          detail::Remove<RemoveComponents...>, ArgTuples&&... _argTuples)
    {
        // validate registrations (guard empty packs)
        if ((sizeof...(AddComponents) > 0 &&
             !areComponentsRegistered<AddComponents...>(m_componentRegistry)) ||
            (sizeof...(RemoveComponents) > 0 &&
             !areComponentsRegistered<RemoveComponents...>(m_componentRegistry)))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto mapIt = m_entityToArchetype.find(_eid);
        if (mapIt == m_entityToArchetype.end()) return; // entity not found (no-op)

        Archetype* oldArch = mapIt->second;
        Byte* oldRowPtr = oldArch->get(_eid);
        if (!oldRowPtr) return; // defensive

        ComponentSignature oldSig = oldArch->signature();

        // compute destination signature
        ComponentSignature newSig = oldSig;
        (newSig.setBit(m_componentRegistry->getID<AddComponents>()), ...);
        (newSig.clearBit(m_componentRegistry->getID<RemoveComponents>()), ...);

        // no-op if signature unchanged
        if (oldSig == newSig) return;

        size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
        Archetype* newArch = getOrCreateArchetype(newSig, newStride);

        if (newArch == oldArch) return; // safety (shouldn't happen, but cheap guard)

        std::vector<Byte> newRow(newStride);
        Byte* newRowPtr = newRow.data();

        // copy EntityMeta
        EntityMeta meta = *reinterpret_cast<EntityMeta*>(oldRowPtr);
        new (newRowPtr) EntityMeta{meta};

        // copy surviving components from old row to the corresponding new offsets
        auto oldOffsets = oldArch->componentOffsets(); // map<CompID, offset>
        auto newOffsets = newArch->componentOffsets(); // map<CompID, offset>

        for (const auto& [compID, srcOffset] : oldOffsets)
        {
            auto destIt = newOffsets.find(compID);
            if (destIt == newOffsets.end()) continue; // component removed
            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + destIt->second, oldRowPtr + srcOffset, compSize);
        }

        // construct newly added components with args (if any)
        if constexpr (sizeof...(AddComponents) > 0)
        {
            (
                [&]<typename T, typename ArgTuple>(ArgTuple&& argTuple)
                {
                    auto offset = newOffsets[m_componentRegistry->getID<T>()];
                    std::apply(
                        [&](auto&&... args)
                        { new (newRowPtr + offset) T(std::forward<decltype(args)>(args)...); },
                        std::forward<ArgTuple>(argTuple));
                }.template operator()<AddComponents>(std::forward<ArgTuples>(_argTuples)),
                ...);
        }

        // perform the migration: insert into destination archetype, remove from source, update
        // mapping
        newArch->insert(meta.id, newRowPtr);
        oldArch->remove(meta.id);
        m_entityToArchetype[meta.id] = newArch;
    }

    /**
     * @brief Adds the specified components to the entity with the given ID, initialized with
     * constructor arguments.
     *
     * @tparam Ts The component types to add.
     * @tparam ArgTuples The tuple types containing constructor arguments for each component.
     * @param _eid The ID of the entity to which components will be added.
     * @param _argTuples Tuples of constructor arguments for each component.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts, typename... ArgTuples>
        requires(sizeof...(Ts) == sizeof...(ArgTuples))
    void addComponents(EntityID _eid, ArgTuples&&... _argTuples)
    {
        modifyComponents(_eid, detail::Add<Ts...>{}, detail::Remove<>{},
                         std::forward<ArgTuples>(_argTuples)...);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Bulk Operations API
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Creates multiple entities with the same component signature.
     *
     * All entities are created with default-initialized components.
     * This is significantly faster than calling createEntity() in a loop.
     *
     * @tparam Ts The component types for all new entities.
     * @param _count Number of entities to create.
     * @return Vector of EntityMeta for all created entities.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    std::vector<EntityMeta> createEntityBulk(size_t _count)
    {
        if (_count == 0) return {};

        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        auto stride = calculateStrideFromSignature(m_componentRegistry, sig);
        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        // Allocate all entity IDs upfront
        std::vector<EntityMeta> metas = m_EntityAllocationHelper.getIDBulk(_count);

        // Get or create archetype
        Archetype* arch = getOrCreateArchetype(sig, stride);

        // Extract just the IDs for bulk insert
        std::vector<EntityID> eids;
        eids.reserve(_count);
        for (const auto& meta : metas)
        {
            eids.push_back(meta.id);
        }

        // Allocate uninitialized storage for all entities
        Byte* dstBase = arch->insertBulkUninitialized(eids.data(), _count);

        // Initialize each entity's data
        for (size_t i = 0; i < _count; ++i)
        {
            Byte* rowPtr = dstBase + i * stride;

            // Initialize EntityMeta
            new (rowPtr) EntityMeta{metas[i]};

            // Default-initialize all components
            ((new (rowPtr + componentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

            // Update entity-to-archetype mapping
            m_entityToArchetype[metas[i].id] = arch;
        }

        return metas;
    }

    /**
     * @brief Creates multiple entities with the same component signature, initialized with
     * constructor arguments.
     *
     * All entities are created with the same constructor arguments (shared initialization).
     * This is significantly faster than calling createEntity() in a loop.
     *
     * @tparam Ts The component types for all new entities.
     * @tparam ArgTuples The tuple types containing constructor arguments for each component.
     * @param _count Number of entities to create.
     * @param _argTuples Tuples of constructor arguments for each component (use std::make_tuple).
     * @return Vector of EntityMeta for all created entities.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts, typename... ArgTuples>
        requires(sizeof...(Ts) == sizeof...(ArgTuples))
    std::vector<EntityMeta> createEntityBulk(size_t _count, ArgTuples&&... _argTuples)
    {
        if (_count == 0) return {};

        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        auto stride = calculateStrideFromSignature(m_componentRegistry, sig);
        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        // Allocate all entity IDs upfront
        std::vector<EntityMeta> metas = m_EntityAllocationHelper.getIDBulk(_count);

        // Get or create archetype
        Archetype* arch = getOrCreateArchetype(sig, stride);

        // Extract just the IDs for bulk insert
        std::vector<EntityID> eids;
        eids.reserve(_count);
        for (const auto& meta : metas)
        {
            eids.push_back(meta.id);
        }

        // Allocate uninitialized storage for all entities
        Byte* dstBase = arch->insertBulkUninitialized(eids.data(), _count);

        // Initialize each entity's data with the SAME constructor arguments
        for (size_t i = 0; i < _count; ++i)
        {
            Byte* rowPtr = dstBase + i * stride;

            // Initialize EntityMeta
            new (rowPtr) EntityMeta{metas[i]};

            // Construct each component with its args via tuple unpacking (same args for all)
            (
                [&]<typename T, typename ArgTuple>(ArgTuple&& argTuple)
                {
                    auto offset = componentOffsets[m_componentRegistry->getID<T>()];
                    std::apply([&](auto&&... args)
                               { new (rowPtr + offset) T(std::forward<decltype(args)>(args)...); },
                               std::forward<ArgTuple>(argTuple));
                }.template operator()<Ts>(_argTuples),
                ...);

            // Update entity-to-archetype mapping
            m_entityToArchetype[metas[i].id] = arch;
        }

        return metas;
    }

    /**
     * @brief Destroys multiple entities.
     *
     * Invalid entity IDs are skipped (not treated as errors).
     *
     * @param _eids Vector of entity IDs to destroy.
     * @return Vector of entity IDs that were NOT found (skipped).
     */
    std::vector<EntityID> destroyEntityBulk(const std::vector<EntityID>& _eids)
    {
        std::vector<EntityID> notFound;

        // Group entities by archetype for batch removal
        std::unordered_map<Archetype*, std::vector<EntityID>> archetypeGroups;

        for (EntityID eid : _eids)
        {
            auto it = m_entityToArchetype.find(eid);
            if (it == m_entityToArchetype.end())
            {
                notFound.push_back(eid);
            }
            else
            {
                archetypeGroups[it->second].push_back(eid);
            }
        }

        // Remove entities from each archetype in batches
        for (auto& [arch, eids] : archetypeGroups)
        {
            arch->removeBulk(eids.data(), eids.size());

            for (EntityID eid : eids)
            {
                m_entityToArchetype.erase(eid);
            }
        }

        // Free all valid IDs at once
        std::vector<EntityID> validIds;
        validIds.reserve(_eids.size() - notFound.size());
        for (EntityID eid : _eids)
        {
            if (std::find(notFound.begin(), notFound.end(), eid) == notFound.end())
            {
                validIds.push_back(eid);
            }
        }
        m_EntityAllocationHelper.freeIDBulk(validIds);

        return notFound;
    }

    /**
     * @brief Modifies components of multiple entities in bulk by adding and/or removing specified
     * components.
     *
     * If an entity already has some of the specified components, they will not be duplicated.
     * If an entity does not have some of the components to be removed, they will be ignored.
     * The entities will be moved to the corresponding archetypes that match their updated component
     * signatures.
     *
     * @tparam AddComponents The component types to be added to the entities.
     * @tparam RemoveComponents The component types to be removed from the entities.
     * @param _eids Vector of entity IDs whose components are to be modified.
     * @return std::vector<EntityID> Vector of entity IDs that were NOT found (skipped).
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... AddComponents, Component... RemoveComponents>
    std::vector<EntityID> modifyComponentsBulk(detail::Add<AddComponents...>,
                                               detail::Remove<RemoveComponents...>,
                                               const std::vector<EntityID>& _eids)
    {
        if (_eids.empty()) return {};

        // validate registrations (guard empty packs)
        if ((sizeof...(AddComponents) > 0 &&
             !areComponentsRegistered<AddComponents...>(m_componentRegistry)) ||
            (sizeof...(RemoveComponents) > 0 &&
             !areComponentsRegistered<RemoveComponents...>(m_componentRegistry)))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        std::vector<EntityID> notFound;
        std::unordered_map<Archetype*, std::vector<EntityID>> archetypeGroups;

        // Group entities by their current archetype (skip missing)
        for (EntityID eid : _eids)
        {
            auto it = m_entityToArchetype.find(eid);
            if (it == m_entityToArchetype.end())
            {
                notFound.push_back(eid);
            }
            else
            {
                archetypeGroups[it->second].push_back(eid);
            }
        }

        // Process each group
        for (auto& [srcArch, eids] : archetypeGroups)
        {
            ComponentSignature oldSig = srcArch->signature();

            // Compute new signature: set bits for Add, clear bits for Remove
            ComponentSignature newSig = oldSig;
            (newSig.setBit(m_componentRegistry->getID<AddComponents>()), ...);
            (newSig.clearBit(m_componentRegistry->getID<RemoveComponents>()), ...);

            if (oldSig == newSig) continue; // nothing to do for this archetype

            size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
            Archetype* dstArch = getOrCreateArchetype(newSig, newStride);

            auto srcOffsets = srcArch->componentOffsets();  // map<CompID, offset>
            auto destOffsets = dstArch->componentOffsets(); // map<CompID, offset>

            for (EntityID eid : eids)
            {
                Byte* srcRow = srcArch->get(eid);
                if (!srcRow) continue; // defensive

                std::vector<Byte> newRow(newStride);
                Byte* newRowPtr = newRow.data();

                // copy meta
                EntityMeta meta = *reinterpret_cast<EntityMeta*>(srcRow);
                new (newRowPtr) EntityMeta{meta};

                // Copy components that survive into destination offsets
                for (const auto& [compID, srcOffset] : srcOffsets)
                {
                    auto it = destOffsets.find(compID);
                    if (it == destOffsets.end()) continue; // this component was removed
                    size_t compSize = m_componentRegistry->info(compID).size;
                    std::memcpy(newRowPtr + it->second, srcRow + srcOffset, compSize);
                }

                // Default-construct newly added components
                if constexpr (sizeof...(AddComponents) > 0)
                {
                    ((new (newRowPtr + destOffsets[m_componentRegistry->getID<AddComponents>()])
                          AddComponents()),
                     ...);
                }

                // Insert into destination archetype, remove from source, update map
                dstArch->insert(meta.id, newRowPtr);
                srcArch->remove(meta.id);
                m_entityToArchetype[meta.id] = dstArch;
            }
        }

        return notFound;
    }

    /**
     * @brief Adds the specified components to multiple entities in bulk.
     *
     * @tparam Ts The component types to add.
     * @param _eids Vector of entities to which components will be added.
     * @return std::vector<EntityID> Vector of entity IDs that were NOT found (skipped).
     */
    template <Component... Ts>
    std::vector<EntityID> addComponentsBulk(const std::vector<EntityID>& _eids)
    {
        return modifyComponentsBulk(detail::Add<Ts...>{}, detail::Remove<>{}, _eids);
    }

    /**
     * @brief Removes the specified components from multiple entities in bulk.
     *
     * @tparam Ts The component types to remove.
     * @param _eids Vector of entities from which components will be removed.
     * @return std::vector<EntityID> Vector of entity IDs that were NOT found (skipped).
     */
    template <Component... Ts>
    std::vector<EntityID> removeComponentsBulk(const std::vector<EntityID>& _eids)
    {
        return modifyComponentsBulk(detail::Add<>{}, detail::Remove<Ts...>{}, _eids);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Archetype Migration API
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    template <Component... SourceComponents, Component... AddComponents,
              Component... RemoveComponents>
    size_t migrateArchetypeModifyComponents(detail::From<SourceComponents...>,
                                            detail::Add<AddComponents...>,
                                            detail::Remove<RemoveComponents...>)
    {
        static_assert(sizeof...(SourceComponents) > 0,
                      "Source archetype must specify at least one component");

        // Verify components are registered (guard empty packs)
        if (!areComponentsRegistered<SourceComponents...>(m_componentRegistry) ||
            (sizeof...(AddComponents) > 0 &&
             !areComponentsRegistered<AddComponents...>(m_componentRegistry)) ||
            (sizeof...(RemoveComponents) > 0 &&
             !areComponentsRegistered<RemoveComponents...>(m_componentRegistry)))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        // Source signature & archetype lookup
        ComponentSignature srcSig = getSignatureFromTypes<SourceComponents...>(m_componentRegistry);

        auto srcIt = m_archetypes.find(srcSig);
        if (srcIt == m_archetypes.end() || srcIt->second->empty()) return 0;

        Archetype* srcArch = srcIt->second.get();

        // Compute destination signature: set bits for Add, clear bits for Remove
        ComponentSignature destSig = srcSig;
        (destSig.setBit(m_componentRegistry->getID<AddComponents>()), ...);
        (destSig.clearBit(m_componentRegistry->getID<RemoveComponents>()), ...);

        if (srcSig == destSig) return 0; // nothing to do

        // Ensure destination archetype exists
        size_t destStride = calculateStrideFromSignature(m_componentRegistry, destSig);
        Archetype* dstArch = getOrCreateArchetype(destSig, destStride);

        auto destOffsets = dstArch->componentOffsets();

        // Count and migrate
        size_t count = srcArch->size();
        std::vector<EntityID> migratedIds = srcArch->migrateAllTo(*dstArch, m_componentRegistry);

        // Initialize added components (if any) and update entity->archetype mapping
        if constexpr (sizeof...(AddComponents) > 0)
        {
            for (EntityID eid : migratedIds)
            {
                Byte* rowPtr = dstArch->get(eid);
                // placement-new each added component at its offset
                ((new (rowPtr + destOffsets[m_componentRegistry->getID<AddComponents>()])
                      AddComponents()),
                 ...);

                m_entityToArchetype[eid] = dstArch;
            }
        }
        else
        {
            // No components to construct; just update mapping
            for (EntityID eid : migratedIds)
            {
                m_entityToArchetype[eid] = dstArch;
            }
        }

        return count;
    }

    /**
     * @brief Clears all entities and archetypes from the registry, resetting the entity allocator.
     */
    void clear()
    {
        m_EntityAllocationHelper.reset();
        m_entityToArchetype.clear();
        m_archetypes.clear();
    }

    /**
     * @brief Retrieves a view of entities that have exactly the specified set of components (strict
     * archetype match).
     *
     * @tparam Ts The component types that define the view.
     * @return std::optional<EntityView<Ts...>> containing the view, or std::nullopt if no
     * archetype has the exact set of requested components.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    [[nodiscard]] std::optional<EntityView<Ts...>> viewSet()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        ComponentSignature sig = getSignatureFromTypes<Ts...>(m_componentRegistry);

        if (m_archetypes.find(sig) == m_archetypes.end()) return std::nullopt;
        if (m_archetypes.at(sig)->empty()) return std::nullopt;

        return EntityView<Ts...>({m_archetypes.at(sig).get()}, m_componentRegistry);
    }

    /**
     * @brief Retrieves a view of entities that have at least the specified set of components
     * (looser archetype match).
     *
     * @tparam Ts The component types that define the view.
     * @return std::optional<EntityView<Ts...>> containing the view, or std::nullopt if no
     * archetypes have a superset of the requested components.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    [[nodiscard]] std::optional<EntityView<Ts...>> viewSubset()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        std::vector<Archetype*> matchingArches;
        ComponentSignature sig = getSignatureFromTypes<Ts...>(m_componentRegistry);

        for (const auto& [archSig, arch] : m_archetypes)
        {
            if ((archSig & sig) == sig && !arch->empty()) matchingArches.push_back(arch.get());
        }

        if (matchingArches.empty()) return std::nullopt;

        return EntityView<Ts...>(matchingArches, m_componentRegistry);
    }

    /**
     * @brief Retrieves the archetype that contains the entity with the specified ID.
     *
     * @param _eid The ID of the entity whose archetype is to be retrieved.
     * @return Archetype* or nullptr if the entity does not exist.
     */
    [[nodiscard]] Archetype* getArchetypeForEntity(EntityID _eid) const
    {
        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return nullptr;

        return m_entityToArchetype.at(_eid);
    }

    /**
     * @brief Retrieves the components of the entity with the specified ID as a tuple of references.
     *
     * If the entity does not have all the specified components, std::nullopt is returned.
     *
     * @tparam Ts The component types to be retrieved.
     * @param _eid The ID of the entity whose components are to be retrieved.
     * @return std::optional<std::tuple<Ts&...>> containing references to the components, or
     * std::nullopt if the entity does not have all specified components.
     * @throws std::runtime_error if one or more components are not registered.
     */
    template <Component... Ts>
    [[nodiscard]] std::optional<std::tuple<Ts&...>> getComponentsForEntity(EntityID _eid)
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto arch = getArchetypeForEntity(_eid);

        if (!arch) return std::nullopt;

        ComponentSignature archSig = arch->signature();
        ComponentSignature querySig = getSignatureFromTypes<Ts...>(m_componentRegistry);

        if ((archSig & querySig) != querySig) return std::nullopt;

        Byte* rowPtr = arch->get(_eid);
        auto componentOffsets = arch->componentOffsets();

        return std::optional<std::tuple<Ts&...>>{
            std::in_place,
            (*reinterpret_cast<Ts*>(rowPtr +
                                    componentOffsets[m_componentRegistry->getID<Ts>()]))...,
        };
    }

    /**
     * @brief Checks if the entity with the specified metadata is valid (exists and generation
     * matches).
     *
     * @param _meta The metadata of the entity to be checked.
     * @return true if the entity is valid, false otherwise.
     */
    [[nodiscard]] bool isEntityValid(EntityMeta _meta) const
    {
        if (m_entityToArchetype.find(_meta.id) == m_entityToArchetype.end()) return false;

        auto rowPtr = m_entityToArchetype.at(_meta.id)->get(_meta.id);
        auto entityMeta = reinterpret_cast<EntityMeta*>(rowPtr);

        return entityMeta->gen == _meta.gen &&
               m_EntityAllocationHelper.getGenForID(_meta.id) == _meta.gen;
    }

    // Returns the total number of entities in the registry.
    [[nodiscard]] size_t entityCount() const { return m_entityToArchetype.size(); }

    // Returns the total number of archetypes in the registry.
    [[nodiscard]] size_t archetypeCount() const { return m_archetypes.size(); }

    // Returns the total memory usage of all archetypes in bytes.
    [[nodiscard]] size_t totalMemoryUsageInBytes() const
    {
        size_t total = 0;

        for (const auto& [sig, arch] : m_archetypes)
        {
            total += arch->memoryUsageInBytes();
        }

        return total;
    }

   private:
    // Retrieves an existing archetype by signature or creates a new one if it doesn't exist.
    Archetype* getOrCreateArchetype(ComponentSignature _signature, size_t _stride)
    {
        if (m_archetypes.find(_signature) != m_archetypes.end())
        {
            return m_archetypes[_signature].get();
        }

        auto componentOffsets =
            getComponentOffsetsInBytesFromSignature(m_componentRegistry, _signature);

        m_archetypes[_signature] =
            std::make_unique<Archetype>(_signature, _stride, componentOffsets);

        return m_archetypes.at(_signature).get();
    }
};

} // namespace ecs
} // namespace saturn
