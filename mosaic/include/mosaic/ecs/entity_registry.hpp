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
    using Byte = uint8_t;

    class EntityAllocator
    {
       public:
        [[nodiscard]] EntityMeta getID()
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

        [[nodiscard]] EntityGen getGenForID(EntityID _eid) const { return m_generations[_eid]; }

       private:
        EntityID m_next = 0;
        std::vector<EntityID> m_freeList;
        std::vector<EntityGen> m_generations;
    };

    template <Component... Ts>
    class EntityView
    {
       private:
        using Byte = uint8_t;

        std::vector<Archetype*> m_archetypes;
        const ComponentRegistry* m_componentRegistry;

       public:
        EntityView(const std::vector<Archetype*>& _archetypes, const ComponentRegistry* _com)
            : m_archetypes(std::move(_archetypes)), m_componentRegistry(_com) {};

       public:
        template <typename Func>
            requires std::is_invocable_r_v<void, Func, EntityMeta, Ts&...>
        void forEach(Func&& _func)
        {
            for (auto* archetype : m_archetypes)
            {
                auto componentOffsets = archetype->componentOffsets();

                Byte* base = archetype->data();
                size_t stride = archetype->stride();
                size_t count = archetype->size();

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
        }

       public:
        struct Iterator
        {
           private:
            const ComponentRegistry* m_componentRegistry;
            std::vector<Archetype*> m_archetypes;
            size_t m_archetypeIndex;
            size_t m_rowIndex;

            // Cached
            Byte* m_base = nullptr;
            size_t m_stride = 0;
            size_t m_count = 0;
            std::unordered_map<ComponentID, size_t> m_componentOffsets;

            void loadArchetype(size_t index)
            {
                if (index >= m_archetypes.size()) return;
                Archetype* arch = m_archetypes[index];
                m_base = arch->data();
                m_stride = arch->stride();
                m_count = arch->size();
                m_componentOffsets = arch->componentOffsets();
            }

            void advanceToValid()
            {
                while (m_archetypeIndex < m_archetypes.size() && m_rowIndex >= m_count)
                {
                    ++m_archetypeIndex;
                    m_rowIndex = 0;
                    if (m_archetypeIndex < m_archetypes.size()) loadArchetype(m_archetypeIndex);
                }
            }

           public:
            Iterator(const ComponentRegistry* _com, std::vector<Archetype*> _archetypes,
                     size_t archetypeIndex, size_t rowIndex)
                : m_componentRegistry(_com),
                  m_archetypes(std::move(_archetypes)),
                  m_archetypeIndex(archetypeIndex),
                  m_rowIndex(rowIndex)
            {
                if (m_archetypeIndex < m_archetypes.size()) loadArchetype(m_archetypeIndex);
                advanceToValid();
            }

            struct EntityMetaComponentTuplePair
            {
                EntityMeta meta;
                std::tuple<Ts&...> components;
            };

            EntityMetaComponentTuplePair operator*() const
            {
                Byte* rowPtr = m_base + m_rowIndex * m_stride;
                EntityID eid = *reinterpret_cast<EntityID*>(rowPtr);

                auto comps = std::forward_as_tuple((*reinterpret_cast<Ts*>(
                    rowPtr + m_componentOffsets.at(m_componentRegistry->getID<Ts>())))...);

                return {EntityMeta{eid, 0u}, comps};
            }

            bool operator!=(const Iterator& _other) const
            {
                return m_archetypeIndex != _other.m_archetypeIndex ||
                       m_rowIndex != _other.m_rowIndex;
            }

            void operator++()
            {
                ++m_rowIndex;
                advanceToValid();
            }
        };

        Iterator begin() { return Iterator(m_componentRegistry, m_archetypes, 0, 0); }
        Iterator end()
        {
            return Iterator(m_componentRegistry, m_archetypes, m_archetypes.size(), 0);
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
    template <Component... Ts>
    EntityMeta createEntity()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        auto sig = getSignatureFromTypes<Ts...>(m_componentRegistry);
        constexpr auto stride = calculateStrideFromSignature(m_componentRegistry, sig);

        EntityMeta meta = m_entityAllocator.getID();
        Archetype* arch = getOrCreateArchetype(sig, stride);

        auto componentOffsets = getComponentOffsetsInBytesFromSignature(m_componentRegistry, sig);

        std::array<char, stride> row{};
        char* rowPtr = row.data();

        new (rowPtr) EntityMeta{meta};

        ((new (rowPtr + componentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

        arch->insert(meta.id, rowPtr);
        m_entityToArchetype[meta.id] = arch;

        return meta;
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

    template <Component... Ts>
    void addComponents(EntityID _eid)
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return;

        Archetype* oldArch = m_entityToArchetype.at(_eid);
        ComponentSignature oldSig = oldArch->signature();

        ComponentSignature newSig = oldSig;
        (newSig.setBit(m_componentRegistry->getID<Ts>()), ...);

        size_t newStride = calculateStrideFromSignature(m_componentRegistry, newSig);
        Archetype* newArch = getOrCreateArchetype(newSig, newStride);

        if (oldArch == newArch) return;

        std::vector<char> newRow(newStride);
        char* newRowPtr = newRow.data();

        EntityMeta meta = *reinterpret_cast<EntityMeta*>(oldArch->get(_eid));
        new (newRowPtr) EntityMeta{meta};

        auto oldComponentOffsets = oldArch->componentOffsets();
        auto newComponentOffsets = newArch->componentOffsets();

        for (const auto& [compID, offset] : oldComponentOffsets)
        {
            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + newComponentOffsets[compID], oldArch->get(_eid) + offset,
                        compSize);
        }

        ((new (newRowPtr + newComponentOffsets[m_componentRegistry->getID<Ts>()]) Ts()), ...);

        newArch->insert(meta.id, newRowPtr);
        oldArch->remove(meta.id);
        m_entityToArchetype[meta.id] = newArch;
    }

    template <Component... Ts>
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

        auto oldComponentOffsets = oldArch->componentOffsets();
        auto newComponentOffsets = newArch->componentOffsets();

        for (const auto& [compID, offset] : oldComponentOffsets)
        {
            if (newComponentOffsets.find(compID) == newComponentOffsets.end()) continue;

            size_t compSize = m_componentRegistry->info(compID).size;
            std::memcpy(newRowPtr + newComponentOffsets[compID], oldArch->get(_eid) + offset,
                        compSize);
        }

        newArch->insert(meta.id, newRowPtr);
        oldArch->remove(meta.id);
        m_entityToArchetype[meta.id] = newArch;
    }

    void clear()
    {
        m_entityAllocator.reset();
        m_entityToArchetype.clear();
        m_archetypes.clear();
    }

    template <Component... Ts>
    [[nodiscard]] std::optional<EntityView<Ts...>> viewSet()
    {
        if (!areComponentsRegistered<Ts...>(m_componentRegistry))
        {
            throw std::runtime_error("One or more components are not registered.");
        }

        ComponentSignature sig = getSignatureFromTypes<Ts...>(m_componentRegistry);

        if (m_archetypes.find(sig) == m_archetypes.end()) return std::nullopt;

        return EntityView<Ts...>({m_archetypes.at(sig).get()}, m_componentRegistry);
    }

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
            if ((archSig & sig) == sig) matchingArches.push_back(arch.get());
        }

        if (matchingArches.empty()) return std::nullopt;

        return EntityView<Ts...>(matchingArches, m_componentRegistry);
    }

    [[nodiscard]] size_t entityCount() const { return m_entityToArchetype.size(); }

    [[nodiscard]] size_t archetypeCount() const { return m_archetypes.size(); }

    [[nodiscard]] Archetype* getArchetypeForEntity(EntityID _eid) const
    {
        if (m_entityToArchetype.find(_eid) == m_entityToArchetype.end()) return nullptr;

        return m_entityToArchetype.at(_eid);
    }

    template <Component... Ts>
    [[nodiscard]] std::optional<std::tuple<Ts&...>> getComponents(EntityID _eid)
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

    [[nodiscard]] bool isEntityValid(EntityMeta _meta) const
    {
        if (m_entityToArchetype.find(_meta.id) == m_entityToArchetype.end()) return false;

        auto rowPtr = m_entityToArchetype.at(_meta.id)->get(_meta.id);
        auto entityMeta = reinterpret_cast<EntityMeta*>(rowPtr);

        return entityMeta->gen == _meta.gen == m_entityAllocator.getGenForID(_meta.id);
    }

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
} // namespace mosaic
