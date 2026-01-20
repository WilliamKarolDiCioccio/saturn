#pragma once

#include <vector>
#include <tuple>
#include <unordered_map>
#include <type_traits>
#include <cstdint>
#include <utility>

#include "entity.hpp"
#include "archetype.hpp"
#include "component_registry.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief The 'EntityView' class provides a non-owning view over a set of entities that share
 * a specific set of components even if they are stored in different archetypes.
 *
 * @tparam Ts The component types that define the view.
 */
template <Component... Ts>
class EntityView
{
   private:
    using Byte = uint8_t;

    std::vector<Archetype*> m_archetypes;
    const ComponentRegistry* m_componentRegistry;

   public:
    /**
     * @brief Constructs an EntityView with the given archetypes and component registry
     * (inherited from the EntityRegistry).
     *
     * @param _archetypes The list of archetypes containing the entities to be viewed.
     * @param _com The component registry for component type information.
     */
    EntityView(const std::vector<Archetype*>& _archetypes, const ComponentRegistry* _com)
        : m_archetypes(std::move(_archetypes)), m_componentRegistry(_com) {};

   public:
    /**
     * @brief Applies the provided function to each entity and its components in the view.
     *
     * @tparam Func The type of the function to be applied.
     * @param _func The function to apply to each entity and its components. It should accept
     * an EntityMeta and references to the components of types Ts...
     */
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
    /**
     * @brief An iterator for traversing entities and their components in the view.
     */
    struct Iterator
    {
       private:
        const ComponentRegistry* m_componentRegistry;
        std::vector<Archetype*> m_archetypes;
        size_t m_archetypeIndex;
        size_t m_rowIndex;

        // Cached values for the current archetype
        Byte* m_base = nullptr;
        size_t m_stride = 0;
        size_t m_count = 0;
        std::unordered_map<ComponentID, size_t> m_componentOffsets;

        // Load the archetype at the given index and update cached values
        void loadArchetype(size_t index)
        {
            if (index >= m_archetypes.size()) return;
            Archetype* arch = m_archetypes[index];
            m_base = arch->data();
            m_stride = arch->stride();
            m_count = arch->size();
            m_componentOffsets = arch->componentOffsets();
        }

        // Advance to the next valid archetype with rows, if necessary
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
            return m_archetypeIndex != _other.m_archetypeIndex || m_rowIndex != _other.m_rowIndex;
        }

        void operator++()
        {
            ++m_rowIndex;
            advanceToValid();
        }
    };

    Iterator begin() { return Iterator(m_componentRegistry, m_archetypes, 0, 0); }
    Iterator end() { return Iterator(m_componentRegistry, m_archetypes, m_archetypes.size(), 0); }
};

} // namespace ecs
} // namespace saturn
