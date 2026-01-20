#pragma once

#include <vector>

#include "entity.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief Helper class for allocating and freeing entity metadata (IDs and generations).
 */
class EntityAllocationHelper
{
   private:
    EntityID m_next = 0;
    std::vector<EntityID> m_freeList;
    std::vector<EntityGen> m_generations;

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

    /**
     * @brief Allocates multiple entity IDs at once.
     *
     * @param _count Number of IDs to allocate.
     * @return Vector of allocated EntityMeta.
     */
    [[nodiscard]] std::vector<EntityMeta> getIDBulk(size_t _count)
    {
        std::vector<EntityMeta> result;
        result.reserve(_count);

        for (size_t i = 0; i < _count; ++i)
        {
            result.push_back(getID());
        }

        return result;
    }

    /**
     * @brief Frees multiple entity IDs at once.
     *
     * @param _ids Vector of entity IDs to free.
     */
    void freeIDBulk(const std::vector<EntityID>& _ids)
    {
        m_freeList.reserve(m_freeList.size() + _ids.size());
        for (EntityID id : _ids)
        {
            m_freeList.push_back(id);
        }
    }

    void reset()
    {
        m_next = 0;
        m_freeList.clear();
        m_generations.clear();
    }

    [[nodiscard]] EntityGen getGenForID(EntityID _eid) const { return m_generations[_eid]; }
};

} // namespace ecs
} // namespace saturn
