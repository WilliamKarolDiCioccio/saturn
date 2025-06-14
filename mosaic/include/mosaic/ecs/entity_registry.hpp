#pragma once

#include <vector>
#include <utility>
#include <tuple>
#include <unordered_map>

#include <pieces/sparse_set.hpp>

#include "entity.hpp"
#include "component.hpp"
#include "archetype.hpp"

namespace mosaic
{
namespace ecs
{

class EntityRegistry
{
   private:
    using CreatorFn = std::function<std::unique_ptr<ArchetypeBase>(ComponentMask)>;

    template <IsComponent... Components>
    using RegistryView = std::vector<std::pair<EntityID, std::tuple<Components...>>>;

    class EntityAllocator
    {
       public:
        EntityID getID()
        {
            if (!m_freeList.empty())
            {
                EntityID id = m_freeList.back();
                m_freeList.pop_back();
                return id;
            }

            return m_next++;
        }

        void freeID(EntityID _id) { m_freeList.push_back(_id); }

        void reset()
        {
            m_next = 0;
            m_freeList.clear();
        }

       private:
        EntityID m_next = 0;
        std::vector<EntityID> m_freeList;
    } m_entityAllocator;

    pieces::SparseSet<EntityID, Entity> m_entities;
    std::unordered_map<ComponentMask, CreatorFn> m_archetypeCreators;
    std::unordered_map<ComponentMask, std::unique_ptr<ArchetypeBase>> m_archetypes;

   public:
    EntityRegistry() = default;

    template <IsComponent... Components>
    EntityID createEntity(Components&&... _components)
    {
        const EntityID eid = m_entityAllocator.getID();

        m_entities.insert(eid, Entity(eid, getComponentMask<Components...>()));

        auto it = m_archetypes.find(getComponentMask<Components...>());

        if (it != m_archetypes.end())
        {
            auto& arch = it->second;
        }
        else
        {
            auto& arch = ensureArchetype<Components...>();
        }

        return eid;
    }

    void destroyEntity(EntityID _eid)
    {
        Entity& entity = m_entities.get(_eid).unwrap();

        const ComponentMask mask = entity.componentMask;

        m_entityAllocator.freeID(_eid);
        m_entities.remove(_eid);

        auto& arch = m_archetypes.at(mask);
    }

    template <IsComponent... Components>
    void moveEntityTo(EntityID _eid, Components&&... _components)
    {
        const ComponentMask mask = getComponentMask<Components...>();

        auto it = m_archetypes.find(mask);

        if (it != m_archetypes.end())
        {
            auto& arch = static_cast<Archetype<Components...>&>(*it->second);

            arch.addEntity(_eid);
        }
        else
        {
            auto& arch = ensureArchetype<Components...>();

            arch.addEntity(_eid);
        }
    }

    template <IsComponent... Components>
    bool hasComponents(EntityID _eid)
    {
        auto entity = m_entities.get(_eid).unwrap();

        return entity.componentMask == getComponentMask<Components...>();
    }

    template <IsComponent... Components>
    RegistryView<Components...> view()
    {
        // const ComponentMask gMask = getComponentMask<Components...>();

        // if (m_archetypes.find(gMask) == m_archetypes.end())
        // {
        //     return;
        // }

        // for (auto& [mask, arch] : m_archetypes)
        // {
        //     if ((mask & gMask) != gMask) continue;
        // }

        return {};
    }

    template <IsComponent... Components>
    void forEach(const std::function<void(Entity, Components&...)>& _func)
    {
        // const ComponentMask gMask = getComponentMask<Components...>();

        // if (m_archetypes.find(gMask) == m_archetypes.end())
        // {
        //     return;
        // }

        // for (auto& [mask, arch] : m_archetypes)
        // {
        //     if ((mask.bits & gMask.bits) != gMask.bits) continue;

        //     for (auto& [eid, components] : arch->entities)
        //     {
        //         auto entity = m_entities.get(eid).unwrap();
        //         _func(entity, std::get<Components>(components)...);
        //     }
        // }
    }

    [[nodiscard]] inline size_t getComponentCount() const { return 0; }

    [[nodiscard]] inline size_t getArchetypeCount() const { return 0; }

   private:
    template <IsComponent... Components>
    ArchetypeBase& ensureArchetype()
    {
        ComponentMask mask = getComponentMask<Components...>();

        if (m_archetypes.find(mask) == m_archetypes.end())
        {
            auto arch = std::make_unique<Archetype<Components...>>();
            m_archetypes[mask] = std::move(arch);
            return *m_archetypes[mask];
        }
    }

    template <IsComponent... Components>
    ComponentMask getComponentMask()
    {
        ComponentMask mask;

        (mask.set(Component<Components>::id), ...);

        return mask;
    }
};

} // namespace ecs
} // namespace mosaic
