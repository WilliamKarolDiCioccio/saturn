#pragma once

#include <array>
#include <vector>
#include <bitset>
#include <memory>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <pieces/core/templates.hpp>

#include "saturn/defines.hpp"

#include "entity.hpp"
#include "typeless_vector.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief A typeless sparse set implementation optimized for ECS component storage.
 *
 * This sparse set stores entity IDs as keys and component data as untyped byte arrays
 * using TypelessVector. It maintains O(1) insertion, deletion, and lookup while being
 * completely type-agnostic at runtime.
 *
 * @tparam PageSize The size of each sparse page (default is 64).
 * @tparam AggressiveReclaim Whether to reclaim memory aggressively (default is false).
 */
template <size_t PageSize = 64, bool AggressiveReclaim = false>
    requires(PageSize > 0)
class TypelessSparseSet final
{
   public:
    using Byte = uint8_t;

   private:
    struct Page
    {
        std::array<size_t, PageSize> sparse{}; // maps entity index to dense index
        std::bitset<PageSize> present{};       // whether entity index is present
        size_t presentCount = 0;               // optimization to avoid popcount

        Page() { present.reset(); }
    };

    std::vector<std::unique_ptr<Page>> m_pages{}; // stores pages of sparse entity IDs
    std::vector<EntityID> m_denseEntities{};      // stores entity IDs densely
    TypelessVector m_componentTuples;             // stores component data using TypelessVector

   public:
    /**
     * @brief Constructs a typeless sparse set with the specified component stride.
     *
     * @param _stride The size in bytes of each component.
     * @param _initialCapacity Initial capacity for components (default is 1).
     * @throws std::invalid_argument if _stride is 0.
     */
    explicit TypelessSparseSet(size_t _stride, size_t _initialCapacity = 1)
        : m_componentTuples(_stride, _initialCapacity)
    {
        if (_stride == 0) throw std::invalid_argument("Component stride must be > 0");
    }

    void insert(EntityID _eid, const void* _componentData)
    {
        Page& page = ensurePageExists(_eid);
        auto offset = getPageOffset(_eid);

        if (!page.present.test(offset))
        {
            // new entity
            page.sparse[offset] = m_denseEntities.size();
            m_denseEntities.push_back(_eid);
            m_componentTuples.pushBack(_componentData);
            page.present.set(offset);
            ++page.presentCount;
        }
        else
        {
            // overwrite entity
            size_t denseIndex = page.sparse[offset];
            Byte* dest = m_componentTuples[denseIndex];
            std::memcpy(dest, _componentData, m_componentTuples.stride());
        }
    }

    bool tryInsert(EntityID _eid, const void* _componentData)
    {
        Page& page = ensurePageExists(_eid);
        auto offset = getPageOffset(_eid);

        if (!page.present.test(offset))
        {
            page.sparse[offset] = m_denseEntities.size();
            m_denseEntities.push_back(_eid);
            m_componentTuples.pushBack(_componentData);
            page.present.set(offset);
            ++page.presentCount;
            return true;
        }

        return false; // entity already exists
    }

    void remove(EntityID _eid)
    {
        if (m_denseEntities.empty()) return;

        const size_t pageIdx = getPageIndex(_eid);

        if (pageIdx >= m_pages.size() || !m_pages[pageIdx]) return;

        Page& page = *m_pages[pageIdx];
        const size_t pageOffset = getPageOffset(_eid);

        if (!page.present.test(pageOffset)) return;

        // Get the dense index of the entity to be removed
        const size_t denseIdx = page.sparse[pageOffset];
        const size_t lastIdx = m_denseEntities.size() - 1;

        if (denseIdx < lastIdx)
        {
            // Get the last entity
            const EntityID lastEntity = m_denseEntities[lastIdx];

            // Swap the last entity with the one being removed
            std::swap(m_denseEntities[denseIdx], m_denseEntities[lastIdx]);
            m_componentTuples.swapElements(denseIdx, lastIdx);

            // Get the last entity's page index and offset
            const size_t lastEntityPageIdx = getPageIndex(lastEntity);
            const size_t lastEntityPageOffset = getPageOffset(lastEntity);

            // Update the sparse array for the last entity
            Page& lastEntityPage = *m_pages[lastEntityPageIdx];
            lastEntityPage.sparse[lastEntityPageOffset] = denseIdx;
        }

        // remove last
        m_denseEntities.pop_back();
        m_componentTuples.popBack();
        page.present.reset(pageOffset);
        --page.presentCount;

        if constexpr (AggressiveReclaim)
        {
            // remove the page if it is empty
            if (page.presentCount == 0) m_pages[pageIdx].reset(nullptr);
        }
    }

    /**
     * @brief Inserts multiple entities with contiguous component data.
     *
     * @param _eids Array of entity IDs to insert.
     * @param _componentData Contiguous buffer of component data (count * stride bytes).
     * @param _count Number of entities to insert.
     *
     * @note Entities that already exist will have their data overwritten.
     */
    void insertBulk(const EntityID* _eids, const void* _componentData, size_t _count)
    {
        if (_count == 0) return;

        // Reserve capacity upfront
        EntityID maxEid = 0;
        for (size_t i = 0; i < _count; ++i)
        {
            if (_eids[i] > maxEid) maxEid = _eids[i];
        }
        reserve(maxEid + 1, m_denseEntities.size() + _count);

        const Byte* srcData = static_cast<const Byte*>(_componentData);
        const size_t stride = m_componentTuples.stride();

        for (size_t i = 0; i < _count; ++i)
        {
            insert(_eids[i], srcData + i * stride);
        }
    }

    /**
     * @brief Inserts multiple entities with uninitialized component data.
     *
     * More efficient than insertBulk when caller will fill data afterward.
     * Returns pointer to first component slot for caller to initialize.
     *
     * @param _eids Array of entity IDs to insert.
     * @param _count Number of entities to insert.
     * @return Pointer to first component data slot, or nullptr if count is 0.
     *
     * @note All entities must be NEW (not already present). Caller must initialize data.
     */
    Byte* insertBulkUninitialized(const EntityID* _eids, size_t _count)
    {
        if (_count == 0) return nullptr;

        // Reserve capacity upfront
        EntityID maxEid = 0;
        for (size_t i = 0; i < _count; ++i)
        {
            if (_eids[i] > maxEid) maxEid = _eids[i];
        }
        reserve(maxEid + 1, m_denseEntities.size() + _count);

        // Reserve dense array space
        m_denseEntities.reserve(m_denseEntities.size() + _count);

        // Append uninitialized component slots
        Byte* destBase = m_componentTuples.appendUninitialized(_count);

        // Update sparse mapping for each entity
        for (size_t i = 0; i < _count; ++i)
        {
            EntityID eid = _eids[i];
            Page& page = ensurePageExists(eid);
            size_t offset = getPageOffset(eid);

            page.sparse[offset] = m_denseEntities.size();
            m_denseEntities.push_back(eid);
            page.present.set(offset);
            ++page.presentCount;
        }

        return destBase;
    }

    /**
     * @brief Removes multiple entities from the sparse set.
     *
     * @param _eids Array of entity IDs to remove.
     * @param _count Number of entities to remove.
     * @return Vector of entity IDs that were NOT found (skip failures).
     */
    std::vector<EntityID> removeBulk(const EntityID* _eids, size_t _count)
    {
        std::vector<EntityID> notFound;

        for (size_t i = 0; i < _count; ++i)
        {
            if (!contains(_eids[i]))
            {
                notFound.push_back(_eids[i]);
            }
            else
            {
                remove(_eids[i]);
            }
        }

        return notFound;
    }

    /**
     * @brief Transfers ALL entities to another sparse set with data transformation.
     *
     * This is the key operation for whole-archetype migration. After this call,
     * this sparse set will be empty and all entities will be in the destination.
     *
     * @param _dest Destination sparse set (must have different stride for component layout change).
     * @param _transform Function to transform source row to dest row: void(EntityID, Byte* src,
     * Byte* dest)
     * @return Vector of all entity IDs that were migrated.
     */
    template <typename TransformFunc>
    std::vector<EntityID> moveAllTo(TypelessSparseSet& _dest, TransformFunc&& _transform)
    {
        if (empty()) return {};

        const size_t count = size();
        std::vector<EntityID> migratedEntities(m_denseEntities);

        // Reserve in destination
        EntityID maxEid = 0;
        for (EntityID eid : m_denseEntities)
        {
            if (eid > maxEid) maxEid = eid;
        }
        _dest.reserve(maxEid + 1, _dest.size() + count);

        // Allocate destination slots
        Byte* destBase = _dest.m_componentTuples.appendUninitialized(count);
        const size_t destStride = _dest.stride();

        // Transform and copy each entity
        for (size_t i = 0; i < count; ++i)
        {
            EntityID eid = m_denseEntities[i];
            Byte* srcRow = m_componentTuples[i];
            Byte* destRow = destBase + i * destStride;

            // Apply transformation (copies shared components, initializes new ones)
            _transform(eid, srcRow, destRow);

            // Update destination sparse mapping
            Page& destPage = _dest.ensurePageExists(eid);
            size_t destOffset = _dest.getPageOffset(eid);
            destPage.sparse[destOffset] = _dest.m_denseEntities.size();
            _dest.m_denseEntities.push_back(eid);
            destPage.present.set(destOffset);
            ++destPage.presentCount;
        }

        // Clear source (but keep allocated memory for potential reuse)
        for (EntityID eid : migratedEntities)
        {
            const size_t pageIdx = getPageIndex(eid);
            if (pageIdx < m_pages.size() && m_pages[pageIdx])
            {
                Page& page = *m_pages[pageIdx];
                size_t offset = getPageOffset(eid);
                page.present.reset(offset);
                --page.presentCount;

                if constexpr (AggressiveReclaim)
                {
                    if (page.presentCount == 0) m_pages[pageIdx].reset(nullptr);
                }
            }
        }
        m_denseEntities.clear();
        m_componentTuples.clear();

        return migratedEntities;
    }

    bool contains(EntityID _eid) const noexcept
    {
        size_t pageIdx = getPageIndex(_eid);
        if (pageIdx >= m_pages.size() || !m_pages[pageIdx]) return false;

        const Page& page = *m_pages[pageIdx];
        return page.present.test(getPageOffset(_eid));
    }

    Byte* get(EntityID _eid) noexcept { return getImpl(*this, _eid); }

    const Byte* get(EntityID _eid) const noexcept { return getImpl(*this, _eid); }

    template <typename T>
    T* getTyped(EntityID _eid) noexcept
    {
        return reinterpret_cast<T*>(get(_eid));
    }

    template <typename T>
    const T* getTyped(EntityID _eid) const noexcept
    {
        return reinterpret_cast<const T*>(get(_eid));
    }

    size_t size() const noexcept { return m_denseEntities.size(); }
    size_t capacity() const noexcept { return m_denseEntities.capacity(); }
    bool empty() const noexcept { return m_denseEntities.empty(); }
    size_t stride() const noexcept { return m_componentTuples.stride(); }

    size_t memoryUsageInBytes() const noexcept
    {
        size_t total = sizeof(*this);
        total += m_denseEntities.capacity() * sizeof(EntityID);
        total += m_componentTuples.memoryUsageInBytes();
        total += m_pages.capacity() * sizeof(std::unique_ptr<Page>);

        for (const auto& pagePtr : m_pages)
        {
            if (pagePtr) total += sizeof(Page);
        }

        return total;
    }

    void reserve(EntityID _maxEntity, size_t _componentCount)
    {
        if constexpr (AggressiveReclaim) return;

        // Reserve pages for entities
        if (_maxEntity > m_pages.size() * PageSize)
        {
            const size_t availablePages = m_pages.size();
            const size_t requiredPages = (_maxEntity + PageSize - 1) / PageSize;

            if (availablePages < requiredPages)
            {
                m_pages.reserve(requiredPages);
                for (size_t i = availablePages; i < requiredPages; ++i)
                {
                    m_pages.emplace_back(std::make_unique<Page>());
                }
            }
        }

        // Reserve space for dense arrays
        if (_componentCount > m_denseEntities.capacity())
        {
            m_denseEntities.reserve(_componentCount);
            m_componentTuples.reserve(_componentCount);
        }
    }

    void clear() noexcept
    {
        m_denseEntities.clear();
        m_componentTuples.clear();
        m_pages.clear();
    }

    void shrinkToFit() noexcept
    {
        m_denseEntities.shrink_to_fit();
        m_componentTuples.shrinkToFit();
        m_pages.shrink_to_fit();
    }

    const std::vector<EntityID>& keys() const noexcept { return m_denseEntities; }
    TypelessVector& data() noexcept { return m_componentTuples; }
    const TypelessVector& data() const noexcept { return m_componentTuples; }

   public:
    struct Iterator
    {
        size_t index;
        const TypelessSparseSet* sparseSet;

        Iterator(size_t _idx, const TypelessSparseSet* _set) : index(_idx), sparseSet(_set) {}

        struct EntityIDComponentTuplePair
        {
            EntityID eid;
            const Byte* component;
        };

        EntityIDComponentTuplePair operator*() const
        {
            return {sparseSet->m_denseEntities[index], sparseSet->m_componentTuples[index]};
        }

        Iterator& operator++()
        {
            ++index;
            return *this;
        }

        bool operator!=(const Iterator& _other) const { return index != _other.index; }
    };

    Iterator begin() { return Iterator(0, this); }
    Iterator end() { return Iterator(size(), this); }

    Iterator begin() const { return Iterator(0, this); }
    Iterator end() const { return Iterator(size(), this); }

   private:
    size_t getPageIndex(EntityID _eid) const { return _eid / PageSize; }

    size_t getPageOffset(EntityID _eid) const { return _eid % PageSize; }

    Page& ensurePageExists(EntityID _eid)
    {
        const size_t pageIdx = getPageIndex(_eid);

        if (pageIdx >= m_pages.size())
        {
            size_t newSize = std::max(pageIdx + 1, m_pages.size() * 2);
            m_pages.resize(newSize);
        }

        if (!m_pages[pageIdx]) m_pages[pageIdx] = std::make_unique<Page>();

        return *m_pages[pageIdx];
    }

    // Constness-abstracted get function to avoid code duplication.
    template <typename Self>
    static auto getImpl(Self& _self, EntityID _eid) noexcept
        -> std::conditional_t<std::is_const_v<Self>, const Byte*, Byte*>
    {
        const size_t pageIdx = _self.getPageIndex(_eid);
        if (pageIdx >= _self.m_pages.size()) return nullptr;

        auto* pagePtr = _self.m_pages[pageIdx].get();
        if (!pagePtr) return nullptr;

        auto& page = *pagePtr;
        const size_t pageOffset = _self.getPageOffset(_eid);

        if (!page.present.test(pageOffset)) return nullptr;

        return _self.m_componentTuples[page.sparse[pageOffset]];
    }
};

} // namespace ecs
} // namespace saturn
