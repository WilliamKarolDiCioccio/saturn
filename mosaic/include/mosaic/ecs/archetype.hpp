#pragma once

#include <bitset>
#include <unordered_map>

#include "component.hpp"
#include "typeless_sparse_set.hpp"

namespace mosaic
{
namespace ecs
{

class Archetype final
{
   private:
    using Byte = uint8_t;

    ComponentSignature m_signature;
    TypelessSparseSet<64, false> m_storage;

   public:
    Archetype(ComponentSignature _signature, size_t _stride)
        : m_signature(_signature), m_storage(_stride) {};

   public:
    void insert(EntityID _eid, const char* _data) { m_storage.insert(_eid, _data); }

    void remove(EntityID _eid) { m_storage.remove(_eid); }

    Byte* get(EntityID _eid) { return m_storage.get(_eid); }

    [[nodiscard]] inline const ComponentSignature& signature() const noexcept
    {
        return m_signature;
    }

    [[nodiscard]] inline size_t stride() const noexcept { return m_storage.stride(); }

    [[nodiscard]] inline size_t size() const noexcept { return m_storage.size(); }

    [[nodiscard]] inline bool empty() const noexcept { return m_storage.empty(); }

    [[nodiscard]] inline Byte* data() noexcept { return m_storage.data().data(); }

    [[nodiscard]] inline size_t memoryUsageInBytes() const noexcept
    {
        return m_storage.memoryUsageInBytes();
    }
};

} // namespace ecs
} // namespace mosaic
