#pragma once

#include <array>

#include <glm/glm.hpp>

#include "component.hpp"

namespace saturn
{
namespace scene
{

struct TagComponent
{
    std::array<char, 32> tag;
    bool active;

    TagComponent() : tag{}, active(true){};

    TagComponent(const std::string& _tag, bool _active = true) : active(_active)
    {
        std::strncpy(tag.data(), _tag.c_str(), tag.size() - 1);
        tag[tag.size() - 1] = '\0'; // Ensure null-termination
    }
};

struct TransformComponent
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    mutable glm::mat4 transform;
    mutable bool dirty;

    TransformComponent()
        : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f), dirty(true){};

    TransformComponent(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
        : position(pos), rotation(rot), scale(scl), dirty(true){};
};

struct CameraComponent
{
    float fov;
    float nearPlane;
    float farPlane;
    bool primary;

    CameraComponent() : fov(45.0f), nearPlane(0.1f), farPlane(100.0f), primary(true) {}

    CameraComponent(float _fov, float _near, float _far, bool _primary = true)
        : fov(_fov), nearPlane(_near), farPlane(_far), primary(_primary){};
};

struct MeshComponent
{
    uint32_t meshId;

    MeshComponent() : meshId(0) {}

    MeshComponent(uint32_t _id) : meshId(_id) {}
};

struct SpriteComponent
{
    uint32_t spriteId;

    SpriteComponent() : spriteId(0) {}

    SpriteComponent(uint32_t _id) : spriteId(_id) {}
};

struct MaterialComponent
{
    uint32_t materialId;

    MaterialComponent() : materialId(0) {}

    MaterialComponent(uint32_t _id) : materialId(_id) {}
};

struct LightComponent
{
    glm::vec3 color;
    float intensity;
    enum class Type
    {
        directional,
        point,
        spot
    } type;

    LightComponent() : color(1.0f), intensity(1.0f), type(Type::point) {}

    LightComponent(const glm::vec3& _color, float _intensity, Type _type)
        : color(_color), intensity(_intensity), type(_type){};
};

} // namespace scene
} // namespace saturn
