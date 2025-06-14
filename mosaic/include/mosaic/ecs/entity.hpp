#pragma once

#include <bitset>

#include "component.hpp"

namespace mosaic
{
namespace ecs
{

using EntityID = uint32_t;

struct Entity
{
    uint32_t id;
    ComponentMask componentMask;

    explicit Entity(uint32_t _id, ComponentMask _componentMask)
        : id(_id), componentMask(_componentMask) {};
};

} // namespace ecs
} // namespace mosaic
