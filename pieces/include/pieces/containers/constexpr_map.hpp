#pragma once

#include <array>
#include <utility>
#include <type_traits>
#include <stdexcept>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{

/**
 * @brief A constexpr map for small fixed-size key-value pairs.
 *
 * NOTE: There is no guarantee that the map will be evaluated at compile time. You should use it in
 * a context where compile-time evaluation is possible.
 *
 *
 * @tparam K The type of the key.
 * @tparam V The type of the value.
 * @tparam Size The number of key-value pairs in the map.
 */
template <typename K, typename V, size_t Size>
struct ConstexprMap final
{
    using Pair = std::pair<K, V>;
    std::array<Pair, Size> data;

    constexpr ConstexprMap(const std::array<Pair, N>& arr) : data(arr) {}

    /**
     * @brief Returns a reference to the value associated with the given key.
     *
     * @param key The key to look up.
     * @return constexpr const RefResult<V, ErrorCode>
     *
     * @see ErrorCode for possible error codes.
     */
    constexpr const Result<V, ErrorCode> at(const K& key) const
    {
        const auto it = std::find_if(data.begin(), data.end(),
                                     [&key](const Pair& pair) { return pair.first == key; });

        if (it != data.end())
        {
            return Ok(it->second);
        }
        else
        {
            return Err(ErrorCode::key_not_found);
        }
    }

    /// Returns the number of key-value pairs in the map.
    static constexpr size_t size() { return Size; }
};

} // namespace pieces
