#pragma once

#include <typeindex>
#include <typeinfo>
#include <string>
#include <vector>
#include <unordered_map>

#include "component.hpp"

namespace saturn
{
namespace ecs
{

/**
 * @brief The 'ComponentRegistry' class manages the registration and metadata of component types
 * within an ECS architecture.
 *
 * It provides functionality to register new component types, retrieve their unique IDs, and
 * access their metadata such as size and alignment.
 *
 * This registry is essential for ensuring that components are correctly identified and managed
 * within the entity registry and archetypes.
 *
 * @note The registry has a maximum capacity for component types, which can be specified at
 * construction time (default is 64) and cannot be exceeded or changed once the registry has been
 * associated with an entity registry as it would invalidate existing component signatures.
 */
class ComponentRegistry final
{
   private:
    size_t m_maxComponents;
    std::unordered_map<std::type_index, ComponentID> m_typeToId;
    std::vector<ComponentMeta> m_infos;

   public:
    /**
     * @brief Constructs a ComponentRegistry with a maximum number of components (default 64).
     *
     * @param _maxComponents The maximum number of distinct component types that can be
     * registered. Default is 64.
     */
    ComponentRegistry(size_t _maxComponents = 64) : m_maxComponents(_maxComponents)
    {
        m_infos.reserve(_maxComponents);
        m_typeToId.reserve(_maxComponents);
    }

   public:
    /**
     * @brief Registers a new component type T with an optional name.
     *
     * @tparam T The component type to be registered.
     * @param name An optional name for the component type. If not provided, the type's
     * name will be used.
     * @return The unique ComponentID assigned to the registered component type.
     * @throws std::runtime_error if the maximum number of components has been exceeded or if
     * the component type is already registered.
     */
    template <typename T>
    ComponentID registerComponent(const std::string& name = typeid(T).name())
    {
        if (m_infos.size() >= m_maxComponents)
        {
            throw std::runtime_error("Exceeded maximum number of components!");
        }

        auto it = m_typeToId.find(std::type_index(typeid(T)));
        if (it != m_typeToId.end()) return it->second;

        ComponentID id = static_cast<ComponentID>(m_infos.size());
        m_typeToId[typeid(T)] = id;
        m_infos.push_back({
            name,
            sizeof(T),
            alignof(T),
        });

        return id;
    }

    /**
     * @brief Retrieves the unique ComponentID for the registered component type T.
     *
     * @tparam T The component type whose ID is to be retrieved.
     * @return The ComponentID of the registered component type T.
     * @throws std::runtime_error if the component type T is not registered.
     */
    template <typename T>
    [[nodiscard]] ComponentID getID() const
    {
        auto it = m_typeToId.find(std::type_index(typeid(T)));
        if (it == m_typeToId.end()) throw std::runtime_error("Component not registered!");
        return it->second;
    }

    /**
     * @brief Retrieves the metadata for the component type with the specified ID.
     *
     * @param id The ComponentID of the component whose metadata is to be retrieved.
     * @return A const reference to the ComponentMeta containing metadata about the component.
     * @throws std::out_of_range if the ComponentID is invalid.
     */
    [[nodiscard]] const ComponentMeta& info(ComponentID id) const
    {
        if (id >= m_infos.size()) throw std::out_of_range("Component ID out of range!");
        return m_infos[id];
    }

    /**
     * @brief Checks if the component type T is registered in the registry.
     *
     * @tparam T The component type to check.
     * @return true if the component type T is registered, false otherwise.
     */
    template <typename T>
    bool isRegistered() const
    {
        return m_typeToId.find(std::type_index(typeid(T))) != m_typeToId.end();
    }

    // Returns the total number of registered component types.
    [[nodiscard]] size_t count() const { return m_infos.size(); }

    // Returns the maximum number of component types that can be registered.
    [[nodiscard]] size_t maxCount() const { return m_maxComponents; }
};

} // namespace ecs
} // namespace saturn
