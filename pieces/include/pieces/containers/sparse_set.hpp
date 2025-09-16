#pragma once

#include <array>
#include <vector>
#include <bitset>
#include <memory>
#include <type_traits>

#include "pieces/internal/error_codes.hpp"
#include "pieces/core/result.hpp"

namespace pieces
{

/**
 * @brief A sparse set implementation that allows for efficient storage and retrieval of key-value
 * pairs, where keys are non-negative integers.
 *
 * Sparse sets offer O1 complexity for insertion, deletion, and lookup operations while paying a
 * price in memory usage. To mitigate this, the implementation uses a page-based approach to store
 * keys and values, allowing for more efficient memory management. This means that the more sparse
 * the set is the more often you'll pay a small price for the allocation of new pages. By using
 * bitsets to implement the sparse presence mask, the implementation can efficiently track empty
 * pages and reclaim memory.
 *
 * @tparam K The type of the keys (must be an unsigned integral type).
 * @tparam T The type of the values.
 * @tparam PageSize The size of each page (default is 64). This should be a positive integer.
 * @tparam AggressiveReclaim Whether to reclaim memory aggressively (default is false).
 */
template <typename K, typename T, size_t PageSize = 64, bool AggressiveReclaim = false>
    requires(std::is_integral_v<K> && std::is_unsigned_v<K> && PageSize > 0)
class SparseSet final
{
   private:
    // Type alias for the sparse set itself to avoid long type names.
    using SelfType = SparseSet<K, T, PageSize, AggressiveReclaim>;

    struct Page
    {
        std::array<size_t, PageSize> sparse{}; // maps key index to dense index
        std::bitset<PageSize> present{};       // whether key index is present
        size_t presentCount = 0;               // optimization to avoid popcount

        Page() { present.reset(); } // initialize all bits to 0
    };

    std::vector<std::unique_ptr<Page>> m_pages{}; // stores pages of sparse keys
    std::vector<K> m_denseKeys{};                 // stores keys densely
    std::vector<T> m_data{};                      // stores values densely

   public:
    SparseSet() = default;

    /**
     * @brief Inserts a key-value pair into the sparse set. If the key already exists, it updates
     * the value.
     *
     * Time complexity: O(1).
     *
     * @param _key The key to insert.
     * @param _value The value to associate with the key.
     *
     * @tparam U The type of the value (must be the same as T when decayed).
     */
    template <typename U>
        requires std::is_same_v<std::decay_t<U>, T>
    void insert(K _key, U&& _value)
    {
        Page& page = ensurePageExists(_key);
        auto offset = getPageOffset(_key);

        if (!page.present.test(offset))
        {
            // new key
            page.sparse[offset] = m_denseKeys.size();
            m_denseKeys.push_back(_key);
            m_data.emplace_back(std::forward<U>(_value));
            page.present.set(offset);
            ++page.presentCount;
        }
        else
        {
            // overwrite existing
            m_data[page.sparse[offset]] = std::forward<U>(_value);
        }
    }

    /**
     * @brief Tries to insert a key-value pair into the sparse set. If the key already exists, it
     * does not update the value.
     *
     * Time complexity: O(1).
     *
     * @param _key The key to insert.
     * @param _value The value to associate with the key.
     *
     * @tparam U The type of the value (must be the same as T when decayed).
     */
    template <typename U>
        requires std::is_same_v<std::decay_t<U>, T>
    [[nodiscard]] bool tryInsert(K _key, U&& _value)
    {
        Page& page = ensurePageExists(_key);
        auto offset = getPageOffset(_key);

        if (!page.present.test(offset))
        {
            // new key
            page.sparse[offset] = m_denseKeys.size();
            m_denseKeys.push_back(_key);
            m_data.emplace_back(std::forward<U>(_value));
            page.present.set(offset);
            ++page.presentCount;
            return true;
        }

        return false; // key already exists
    }

    /**
     * @brief Removes a key-value pair from the sparse set.
     *
     * This function might trigger a memory reclaim if the page is empty and aggressive reclaim is
     * enabled.
     *
     * Time complexity: O(1).
     *
     * @param _key The key to remove.
     */
    void remove(K _key)
    {
        if (m_denseKeys.empty()) return;

        const size_t pageIdx = getPageIndex(_key);

        if (pageIdx >= m_pages.size() || !m_pages[pageIdx]) return;

        Page& page = *m_pages[pageIdx];

        const size_t pageOffset = getPageOffset(_key);

        if (!page.present.test(pageOffset)) return;

        // Get the dense index of the key to be removed and the index to swap with
        const size_t denseIdx = page.sparse[getPageOffset(_key)];
        const size_t lastIdx = m_denseKeys.size() - 1;

        if (denseIdx < lastIdx)
        {
            // Get the last key
            const K lastKey = m_denseKeys[lastIdx];

            // Swap the last key with the one being removed
            std::swap(m_denseKeys[denseIdx], m_denseKeys[lastIdx]);
            std::swap(m_data[denseIdx], m_data[lastIdx]);

            // Get the last key's page index and offset
            const size_t lastKeyPageIdx = getPageIndex(lastKey);
            const size_t lastKeyPageOffset = getPageOffset(lastKey);

            // Update the sparse array for the last key
            Page& lastKeyPage = *m_pages[lastKeyPageIdx];
            lastKeyPage.sparse[lastKeyPageOffset] = denseIdx;
        }

        // remove last
        m_denseKeys.pop_back();
        m_data.pop_back();
        page.present.reset(pageOffset);
        --page.presentCount;

        if constexpr (AggressiveReclaim)
        {
            // remove the page if it is empty
            if (page.presentCount == 0) m_pages[pageIdx].reset(nullptr);
        }
    }

    /**
     * @brief Checks if the sparse set contains a key.
     *
     * @param _key The key to check for.
     * @return true if the key is present, false otherwise.
     */
    [[nodiscard]] bool contains(K _key) const noexcept
    {
        size_t pageIdx = getPageIndex(_key);
        if (pageIdx >= m_pages.size() || !m_pages[pageIdx]) return false;

        const Page& page = *m_pages[pageIdx];
        return page.present.test(getPageOffset(_key));
    }

    /**
     * @brief Retrieves the value associated with a key. Returns an error if the key is not found
     * (non const version).
     *
     * @param _key The key to retrieve the value for.
     * @return RefResult<T, ErrorCode> The result containing the value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] RefResult<T, ErrorCode> get(K _key) noexcept { return getImpl(*this, _key); }

    /**
     * @brief Retrieves the value associated with a key. Returns an error if the key is not found
     * (const version).
     *
     * @param _key The key to retrieve the value for.
     * @return RefResult<const T, ErrorCode> The result containing the value or an error code.
     *
     * @see ErrorCode for possible error codes.
     */
    [[nodiscard]] RefResult<const T, ErrorCode> get(K _key) const noexcept
    {
        return getImpl(*this, _key);
    }

    // Returns the number of keys in the sparse set (gets the dense keys size).
    [[nodiscard]] size_t size() const noexcept { return m_denseKeys.size(); }

    // Wether the sparse set is empty or not (cheking the dense keys size).
    [[nodiscard]] bool empty() const noexcept { return m_denseKeys.empty(); }

    // Unindexes all keys and values from the sparse set.
    void clear() noexcept
    {
        m_denseKeys.clear();
        m_data.clear();
        m_pages.clear();
    }

    // Removes unindexed keys and values from the sparse set to release memory.
    void shrinkToFit() noexcept
    {
        m_denseKeys.shrink_to_fit();
        m_data.shrink_to_fit();
        m_pages.shrink_to_fit();
    }

    /**
     * @brief Pre-allocates memory for the sparse set to accommodate a maximum keys and values
     * number.
     *
     * This can help reduce the number of allocations and improve performance when you know the
     * number of keys and values in advance. If aggressive reclaim is enabled, this function
     * is does not do anything.
     *
     * @param _maxKey The maximum key value to reserve space for.
     * @param _count The number of values to reserve space for.
     */
    void reserve(size_t _maxKey, size_t _count)
    {
        if constexpr (AggressiveReclaim) return;

        // Reserve pages for keys
        if (_maxKey > m_pages.size() * PageSize)
        {
            const size_t availablePages = m_pages.size();
            const size_t requiredPages = (_maxKey + PageSize - 1) / PageSize;

            if (availablePages < requiredPages) m_pages.reserve(requiredPages);

            const size_t pagesToAllocate = requiredPages - availablePages;

            for (size_t i = 0; i < pagesToAllocate; ++i)
            {
                m_pages.emplace_back(std::make_unique<Page>());
            }
        }

        // Reserve space for dense arrays
        if (_count > m_denseKeys.capacity() || _count > m_data.capacity())
        {
            m_denseKeys.reserve(_count);
            m_data.reserve(_count);
        }
    }

    /**
     * @brief Computes the intersection of two sparse sets, returning a new sparse set containing
     * the common keys and values.
     *
     * Time complexity: O(n + m), where n is the size of the current set and m is the size of the
     * other set.
     *
     * @param _other The other sparse set to intersect with.
     * @return SelfType A new sparse set containing the intersection of the two sets.
     */
    [[nodiscard]] SelfType getIntersection(const SelfType& _other)
    {
        SelfType intersectionSet;

        if (size() < _other.size())
        {
            for (const auto& key : m_denseKeys)
            {
                if (_other.contains(key)) intersectionSet.insert(key, m_data[key]);
            }
        }
        else
        {
            for (const auto& key : _other.m_denseKeys)
            {
                if (contains(key)) intersectionSet.insert(key, m_data[key]);
            }
        }

        return intersectionSet;
    }

    /**
     * @brief Computes the union of two sparse sets, returning a new sparse set containing all keys
     * and values from both sets.
     *
     * Time complexity: O(n + m), where n is the size of the current set and m is the size of the
     * other set.
     *
     * @param _other The other sparse set to merge with.
     * @return SelfType A new sparse set containing the merged keys and values.
     */
    [[nodiscard]] SelfType getUnion(const SelfType& _other)
    {
        SelfType unionSet;

        if constexpr (AggressiveReclaim) unionSet.reserve(size() + _other.size());

        for (const auto& key : m_denseKeys) unionSet.insert(key, m_data[key]);
        for (const auto& key : _other.m_denseKeys) unionSet.insert(key, _other.m_data[key]);

        return unionSet;
    }

    /**
     * @brief Computes the difference of two sparse sets, returning a new sparse set containing keys
     * that are in this set but not in the other set.
     *
     * Time complexity: O(n), where n is the size of the current set.
     *
     * @param _other The other sparse set to compute the difference with.
     * @return SelfType A new sparse set containing the difference of the two sets.
     */
    [[nodiscard]] SelfType getDifference(const SelfType& _other) const
    {
        SelfType differenceSet;

        for (const auto& key : m_denseKeys)
        {
            if (!_other.contains(key))
            {
                auto result = get(key);
                if (result.isOk()) differenceSet.insert(key, result.getValue());
            }
        }

        return differenceSet;
    }

    /**
     * @brief Computes the symmetric difference (XOR) of two sparse sets, returning a new sparse set
     * containing keys that are in either set but not in both.
     *
     * Time complexity: O(n + m), where n is the size of the current set and m is the size of the
     * other set.
     *
     * @param _other The other sparse set to compute the symmetric difference with.
     * @return SelfType A new sparse set containing the symmetric difference of the two sets.
     */
    [[nodiscard]] SelfType getSymmetricDifference(const SelfType& _other) const
    {
        SelfType symmetricDifferenceSet;

        for (const auto& key : m_denseKeys)
        {
            if (!_other.contains(key))
            {
                auto result = get(key);
                if (result.isOk()) symmetricDifferenceSet.insert(key, result.getValue());
            }
        }

        for (const auto& key : _other.m_denseKeys)
        {
            if (!contains(key))
            {
                auto result = _other.get(key);
                if (result.isOk()) symmetricDifferenceSet.insert(key, result.getValue());
            }
        }

        return symmetricDifferenceSet;
    }

    /**
     * @brief Checks if this set is a superset of another set.
     */
    [[nodiscard]] bool isSupersetOf(const SelfType& other) const noexcept
    {
        if (other.size() > size()) return false;

        for (const auto& key : other.m_denseKeys)
        {
            if (!contains(key)) return false;
        }

        return true;
    }

    /**
     * @brief Checks if this set is a proper superset of another set.
     */
    [[nodiscard]] bool isProperSupersetOf(const SelfType& other) const noexcept
    {
        return size() > other.size() && isSupersetOf(other);
    }

    /**
     * @brief Checks if this set is a subset of another set.
     */
    [[nodiscard]] bool isSubsetOf(const SelfType& other) const noexcept
    {
        if (size() > other.size()) return false;

        for (const auto& key : m_denseKeys)
        {
            if (!other.contains(key)) return false;
        }

        return true;
    }

    /**
     * @brief Checks if this set is a proper subset of another set.
     */
    [[nodiscard]] bool isProperSubsetOf(const SelfType& other) const noexcept
    {
        return size() < other.size() && isSubsetOf(other);
    }

    /**
     * @brief Checks if this set has no elements in common with another set.
     */
    [[nodiscard]] bool isDisjointWith(const SelfType& other) const noexcept
    {
        for (const auto& key : m_denseKeys)
        {
            if (other.contains(key)) return false;
        }

        return true;
    }

    const std::vector<K>& keys() const noexcept { return m_denseKeys; }
    std::vector<T>& values() noexcept { return m_data; }
    const std::vector<T>& values() const noexcept { return m_data; }

   public:
    /**
     * @brief Iterator for iterating over key-value pairs in the SparseSet.
     */
    struct Iterator
    {
        size_t index;
        const SparseSet* sparseSet;

        Iterator(size_t _idx, const SparseSet* _set) : index(_idx), sparseSet(_set) {}

        struct KeyValuePair
        {
            K key;
            const T& value;
        };

        KeyValuePair operator*() const
        {
            return {sparseSet->m_denseKeys[index], sparseSet->m_data[index]};
        }

        Iterator& operator++()
        {
            ++index;
            return *this;
        }

        bool operator!=(const Iterator& _other) const { return index != _other.index; }
    };

    Iterator begin() { return Iterator(0, this); }
    Iterator end() { return Iterator(size(), this); }

    Iterator begin() const { return Iterator(0, this); }
    Iterator end() const { return Iterator(size(), this); }

   private:
    // Get the corresponding page for the given key.
    [[nodiscard]] size_t getPageIndex(K _key) const { return _key / PageSize; }

    // Get the offset within the page for the given key.
    [[nodiscard]] size_t getPageOffset(K _key) const { return _key % PageSize; }

    // Create or get the page for the given key.
    [[nodiscard]] Page& ensurePageExists(size_t _size)
    {
        const size_t pageIdx = getPageIndex(_size);

        if (pageIdx >= m_pages.size())
        {
            // grow to either exactly what we need, or double the current size whichever is larger
            size_t newSize = std::max(pageIdx + 1, m_pages.size() * 2);
            m_pages.resize(newSize);
        }

        if (!m_pages[pageIdx]) m_pages[pageIdx] = std::make_unique<Page>();

        return *m_pages[pageIdx];
    }

    // Constness abstracted get function to avoid code duplication.
    template <typename Self>
    [[nodiscard]] static auto getImpl(Self& _self, K _key) noexcept
        -> RefResult<std::conditional_t<std::is_const_v<Self>, const T, T>, ErrorCode>
    {
        const size_t pageIdx = _self.getPageIndex(_key);

        if (pageIdx >= _self.m_pages.size())
        {
            return ErrRef<std::conditional_t<std::is_const_v<Self>, const T, T>, ErrorCode>(
                ErrorCode::out_of_range);
        }

        auto* pagePtr = _self.m_pages[pageIdx].get();
        if (!pagePtr)
        {
            return ErrRef<std::conditional_t<std::is_const_v<Self>, const T, T>, ErrorCode>(
                ErrorCode::key_not_found);
        }

        auto& page = *pagePtr;

        if (!page.present.test(_self.getPageOffset(_key)))
        {
            return ErrRef<std::conditional_t<std::is_const_v<Self>, const T, T>, ErrorCode>(
                ErrorCode::key_not_found);
        }

        return OkRef<std::conditional_t<std::is_const_v<Self>, const T, T>, ErrorCode>(
            _self.m_data[page.sparse[_self.getPageOffset(_key)]]);
    }
};

} // namespace pieces
