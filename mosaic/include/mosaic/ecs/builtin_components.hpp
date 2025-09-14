#pragma once

#include <array>

#include <glm/glm.hpp>

#include "component.hpp"

namespace mosaic
{
namespace ecs
{

struct TagComponent
{
    std::array<char, 32> tag;
    bool active;
};

struct TransformComponent
{
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

struct CameraComponent
{
    float fov;
    float nearPlane;
    float farPlane;
    bool primary;
};

struct MeshComponent
{
    uint32_t meshId;
};

struct SpriteComponent
{
    uint32_t spriteId;
};

struct MaterialComponent
{
    uint32_t materialId;
};

struct LightComponent
{
    glm::vec3 color;
    float intensity;
    enum class Type
    {
        Directional,
        Point,
        Spot
    } type;
};

} // namespace ecs
} // namespace mosaic
