#pragma once

#include <typeindex>
#include <typeinfo>
#include <string>
#include <unordered_map>

#include "component.hpp"

namespace mosaic
{
namespace ecs
{

class ComponentRegistry final
{
   private:
    size_t m_maxComponents;
    std::unordered_map<std::type_index, ComponentID> m_typeToId;
    std::vector<ComponentMeta> m_infos;

   public:
    ComponentRegistry(size_t _maxComponents = 64) : m_maxComponents(_maxComponents)
    {
        m_infos.reserve(_maxComponents);
        m_typeToId.reserve(_maxComponents);
    }

   public:
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

    template <typename T>
    [[nodiscard]] ComponentID getID() const
    {
        auto it = m_typeToId.find(std::type_index(typeid(T)));
        if (it == m_typeToId.end()) throw std::runtime_error("Component not registered!");
        return it->second;
    }

    const ComponentMeta& info(ComponentID id) const
    {
        if (id >= m_infos.size()) throw std::out_of_range("Component ID out of range!");
        return m_infos[id];
    }

    template <typename T>
    bool isRegistered() const
    {
        return m_typeToId.find(std::type_index(typeid(T))) != m_typeToId.end();
    }

    size_t count() const { return m_infos.size(); }

    size_t maxCount() const { return m_maxComponents; }
};

} // namespace ecs
} // namespace mosaic
