#pragma once

namespace saturn
{
namespace ecs
{

using EntityID = uint32_t;
using EntityGen = uint32_t;

struct EntityMeta
{
    EntityID id;
    EntityGen gen;
};

} // namespace ecs
} // namespace saturn
