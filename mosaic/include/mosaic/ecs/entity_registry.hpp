#pragma once

#include <array>
#include <functional>
#include <algorithm>
#include <optional>
#include <unordered_map>

#include <pieces/containers/sparse_set.hpp>

#include "entity.hpp"
#include "component.hpp"
#include "helpers.hpp"
#include "archetype.hpp"
#include "component_registry.hpp"

namespace mosaic
{
namespace ecs
{

class EntityRegistry final
{
   private:
    class EntityAllocator
    {
       public:
        EntityMeta getID()
        {
            if (!m_freeList.empty())
            {
                EntityID id = m_freeList.back();
                m_freeList.pop_back();

                ++m_generations[id];

                return {id, m_generations[id]};
            }

            EntityID id = m_next++;

            if (id >= m_generations.size()) m_generations.push_back(0);

            return {id, m_generations[id]};
        }

        void freeID(EntityID _id) { m_freeList.push_back(_id); }

        void reset()
        {
            m_next = 0;
            m_freeList.clear();
            m_generations.clear();
        }

       private:
        EntityID m_next = 0;
        std::vector<EntityID> m_freeList;
        std::vector<EntityGen> m_generations;
    };

    template <typename... Ts>
    class EntityView
    {
       private:
        using Byte = uint8_t;

       private:
        Archetype* m_archetype;
        const ComponentRegistry* m_componentRegistry;

       public:
        EntityView(Archetype* _archetype, const ComponentRegistry* _com)
            : m_archetype(_archetype), m_componentRegistry(_com) {};

       public:
        void forEach(std::function<void(EntityMeta, Ts&...)> _func)
        {
            auto sig = m_archetype->signature();
            auto componentOffsets =
                getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

            Byte* base = m_archetype->data();
            size_t stride = m_archetype->stride();
            size_t count = m_archetype->size();

            for (size_t row = 0; row < count; ++row)
            {
                Byte* rowPtr = base + row * stride;

                EntityMeta meta = *reinterpret_cast<EntityMeta*>(rowPtr);

                auto tuple = std::forward_as_tuple(
                    meta, (*reinterpret_cast<Ts*>(
                              rowPtr + componentOffsets[m_componentRegistry->getID<Ts>()]))...);

                std::apply(_func, tuple);
            }
        }

       public:
        struct Iterator
        {
           private:
            Byte* m_base;
            size_t m_stride;
            size_t m_index;
            size_t m_count;
            const ComponentRegistry* m_componentRegistry;
            std::unordered_map<ComponentID, size_t> m_componentOffsets;

           public:
            Iterator(const ComponentRegistry* _componentRegistry, Byte* _base, size_t _stride,
                     size_t _index, size_t _count)
                : m_componentRegistry(_componentRegistry),
                  m_base(_base),
                  m_stride(_stride),
                  m_index(_index),
                  m_count(_count)
            {
                auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
                m_componentOffsets =
                    getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);
            };

            struct EntityMetaComponentTuplePair
            {
                EntityMeta meta;
                std::tuple<Ts&...> components;
            };

            EntityMetaComponentTuplePair operator*() const
            {
                Byte* rowPtr = m_base + m_index * m_stride;

                EntityID eid = *reinterpret_cast<EntityID*>(rowPtr);

                auto comps = std::forward_as_tuple((*reinterpret_cast<Ts*>(
                    rowPtr + m_componentOffsets.at(m_componentRegistry->getID<Ts>())))...);

                return {EntityMeta{eid, 0u}, comps};
            }

            bool operator!=(const Iterator& _other) const { return m_index != _other.m_index; }

            void operator++() { ++m_index; }
        };

        Iterator begin()
        {
            return Iterator(m_componentRegistry, m_archetype->data(), m_archetype->stride(), 0,
                            m_archetype->size());
        }

        Iterator end()
        {
            return Iterator(m_componentRegistry, m_archetype->data(), m_archetype->stride(),
                            m_archetype->size(), m_archetype->size());
        }
    };

    std::unordered_map<ComponentSignature, std::unique_ptr<Archetype>> m_archetypes;
    std::unordered_map<EntityID, Archetype*> m_entityToArchetype;
    const ComponentRegistry* m_componentRegistry;
    EntityAllocator m_entityAllocator;

   public:
    EntityRegistry(const ComponentRegistry* _componentRegistry)
        : m_componentRegistry(_componentRegistry) {};

   public:
    template <typename... Ts>
    EntityID createEntity()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        constexpr auto stride = getStrideSizeInBytes<Ts...>();

        EntityMeta meta = m_entityAllocator.getID();
        Archetype* arch = getOrCreateArchetype(sig, stride);

        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        std::array<char, stride> row{};
        char* rowPtr = row.data();

        new (rowPtr) EntityMeta{meta};

        ((new (rowPtr + componentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

        arch->insert(meta.eid, rowPtr);
        m_entityToArchetype[meta.eid] = arch;

        return meta.eid;
    }

    void destroyEntity(EntityID _eid)
    {
        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return;

        Archetype* arch = m_entityToArchetype.at(_eid);

        arch->remove(_eid);
        m_entityToArchetype.erase(_eid);
        m_entityAllocator.freeID(_eid);
    }

    void destroyEntities(const std::vector<EntityID>& _eids)
    {
        for (EntityID eid : _eids) destroyEntity(eid);
    }

    template <typename... Ts>
    void addComponents(EntityID _eid)
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return;

        Archetype* oldArch = m_entityToArchetype.at(_eid);
        ComponentSignature oldSig = oldArch->signature();
        size_t oldStride = oldArch->stride();

        ComponentSignature newSig = oldSig;
        (newSig.setBit(m_componentRegistry->getID<Ts>()), ...);

        size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
        Archetype* newArch = getOrCreateArchetype(newSig, newStride);

        if (oldArch == newArch) return;

        std::vector<char> newRow(newStride);
        char* newRowPtr = newRow.data();

        EntityMeta meta = *reinterpret_cast<EntityMeta*>(oldArch->get(_eid));
        new (newRowPtr) EntityMeta{meta};

        auto oldComponentOffsets =
            getComponentOffsetsInBytesFromSignature(m_componentRegistry, oldSig);
        auto newComponentOffsets =
            getComponentOffsetsInBytesFromSignature(m_componentRegistry, newSig);

        for (const auto& [compID, offset] : oldComponentOffsets)
        {
            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + newComponentOffsets[compID], oldArch->get(_eid) + offset,
                        compSize);
        }

        ((new (newRowPtr + newComponentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

        newArch->insert(meta.eid, newRowPtr);
        oldArch->remove(meta.eid);
        m_entityToArchetype[meta.eid] = newArch;
    }

    template <typename... Ts>
    void removeComponents(EntityID _eid)
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return;

        Archetype* oldArch = m_entityToArchetype.at(_eid);
        ComponentSignature oldSig = oldArch->signature();
        size_t oldStride = oldArch->stride();

        ComponentSignature newSig = oldSig;
        (newSig.clearBit(m_componentRegistry->getID<Ts>()), ...);

        size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
        Archetype* newArch = getOrCreateArchetype(newSig, newStride);

        if (oldArch == newArch) return;

        std::vector<char> newRow(newStride);
        char* newRowPtr = newRow.data();

        EntityMeta meta = *reinterpret_cast<EntityMeta*>(oldArch->get(_eid));
        new (newRowPtr) EntityMeta{meta};

        auto oldComponentOffsets =
            getComponentOffsetsInBytesFromSignature(m_componentRegistry, oldSig);
        auto newComponentOffsets =
            getComponentOffsetsInBytesFromSignature(m_componentRegistry, newSig);

        for (const auto& [compID, offset] : oldComponentOffsets)
        {
            if (newComponentOffsets.find(compID) == newComponentOffsets.end()) continue;

            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + newComponentOffsets[compID], oldArch->get(_eid) + offset,
                        compSize);
        }

        newArch->insert(meta.eid, newRowPtr);
        oldArch->remove(meta.eid);
        m_entityToArchetype[meta.eid] = newArch;
    }

    void clear()
    {
        m_entityAllocator.reset();
        m_entityToArchetype.clear();
        m_archetypes.clear();
    }

    size_t entityCount() const { return m_entityToArchetype.size(); }

    size_t archetypeCount() const { return m_archetypes.size(); }

    size_t totalMemoryUsageInBytes() const
    {
        size_t total = 0;

        for (const auto& [sig, arch] : m_archetypes)
        {
            total += arch->memoryUsageInBytes();
        }

        return total;
    }

    template <typename... Ts>
    std::optional<EntityView<Ts...>> view()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        ComponentSignature sig = getSignatureFromTypes<Ts...>(m_componentRegistry);

        if (m_archetypes.find(sig) == m_archetypes.end()) return std::nullopt;

        return EntityView<Ts...>(m_archetypes.at(sig).get(), m_componentRegistry);
    }

   private:
    Archetype* getOrCreateArchetype(ComponentSignature _signature, size_t _stride)
    {
        if (m_archetypes.find(_signature) != m_archetypes.end())
        {
            return m_archetypes[_signature].get();
        }

        m_archetypes[_signature] = std::make_unique<Archetype>(_signature, _stride);

        return m_archetypes.at(_signature).get();
    }
};

} // namespace ecs
} // namespace mosaic
