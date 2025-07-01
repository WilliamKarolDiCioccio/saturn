#pragma once

#include <bitset>
#include <vector>

#include <pieces/sparse_set.hpp>

#include "entity.hpp"
#include "component.hpp"

namespace mosaic
{
namespace ecs
{

constexpr uint16_t k_maxPageSize = 1024;

class ArchetypeBase
{
   public:
    virtual ~ArchetypeBase() = default;
};

template <IsComponent... Components>
struct Archetype : ArchetypeBase
{
    using ComponentStorage =
        pieces::SparseSet<EntityID, std::tuple<EntityID, Components...>, 64, true>;

    ComponentMask componentMask;
    ComponentStorage entities;
};

} // namespace ecs
} // namespace mosaic
