#pragma once

#include <unordered_map>
#include <optional>
#include <shared_mutex>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{
namespace tsafe
{

/**
 * @brief Thread-safe map wrapper around std::unordered_map.
 *
 * This class provides a thread-safe interface for inserting, retrieving,
 * and modifying key-value pairs in an unordered map. It uses a shared mutex
 * to allow multiple threads to read the map concurrently while ensuring
 * exclusive access for write operations. The object is not copyable or movable.
 *
 * @tparam K Key type.
 * @tparam V Value type.
 */
template <typename K, typename V>
class ThreadSafeMap
{
   private:
    std::unordered_map<K, V> m_map;
    mutable std::shared_mutex m_mutex;

   public:
    ThreadSafeMap() = default;

   public:
    /**
     * @brief Insert a key-value pair into the map.
     *
     * @param _key The key to insert.
     * @param _value The value to insert.
     */
    void insert(const K& _key, const V& _value)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_map[_key] = _value;
    }

    /**
     * @brief Insert a key-value pair into the map with move semantics.
     *
     * @param _key The key to insert.
     * @param _value The rvalue reference to insert.
     */
    void emplace(const K& _key, V&& _value)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_map[_key] = std::move(_value);
    }

    /**
     * @brief Insert a key-value pair into the map if the key is not already present.
     *
     * @param _key The key to insert.
     * @param _value The value to insert.
     */
    void insertIfAbsent(const K& _key, const V& _value)
    {
        std::unique_lock lock(m_mutex);
        if (m_map.find(_key) == m_map.end())
        {
            m_map[_key] = _value;
        }
    }

    /**
     * @brief Insert all key-value pairs from another map into this map.
     *
     * This is a transactional operation. If an exception occurs during the insertion,
     * the changes are rolled back to the state before the operation started.
     *
     * @param _other The other map to insert from.
     */
    void insertAll(const std::unordered_map<K, V>& _other)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto backup = m_map;

        try
        {
            for (const auto& [key, value] : _other)
            {
                m_map[key] = value;
            }
        }
        catch (...)
        {
            m_map = std::move(backup);
            throw;
        }
    }

    /**
     * @brief Iterate over the map and apply a function to each key-value pair.
     *
     * This is a transactional operation. If an exception occurs during the iteration,
     * the changes are rolled back to the state before the operation started.
     *
     * @tparam Func The type of the function to apply.
     * @param _func The function to apply to each key-value pair.
     */
    template <typename Func>
    void transform(Func _func)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto backup = m_map;

        try
        {
            _func(m_map);
        }
        catch (...)
        {
            m_map = std::move(backup);
            throw;
        }
    }

    /**
     * @brief Get the value associated with a key.
     *
     * @param _key The key to look up.
     * @return Result<V, ErrorCode> The value associated with the key, or an error code if the key
     * is not found.
     */
    [[nodiscard]] Result<V, ErrorCode> get(const K& _key) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_map.find(_key);

        if (it != m_map.end())
        {
            return Ok<V, ErrorCode>(V(it->second));
        }

        return Err<V, ErrorCode>(ErrorCode::key_not_found);
    }

    /**
     * @brief Check if the map contains a key.
     *
     * @param _key The key to check.
     * @return true if the key is found in the map
     * @return false if the key is not found in the map
     */
    [[nodiscard]] bool contains(const K& _key) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_map.find(_key) != m_map.end();
    }

    /**
     * @brief Remove a key-value pair from the map.
     *
     * @param _key The key to remove.
     * @return true if the key was found and removed
     * @return false if the key was not found in the map
     */
    [[nodiscard]] bool erase(const K& _key)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        return m_map.erase(_key) > 0;
    }

    [[nodiscard]] bool empty() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_map.empty();
    }

    [[nodiscard]] size_t size() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_map.size();
    }

    void clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_map.clear();
    }
};

} // namespace tsafe
} // namespace pieces
