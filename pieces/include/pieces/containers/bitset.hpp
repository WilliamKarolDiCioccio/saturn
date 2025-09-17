#pragma once

#include <bit>
#include <cstring>
#include <cassert>
#include <stdexcept>

#include <pieces/memory/contiguous_allocator.hpp>

namespace pieces
{

/**
 * @brief A BitSet class that provides a dynamic array of bits.
 *
 * This class allows for efficient bit manipulation, including setting, clearing,
 * flipping bits, and performing bitwise operations.
 */
class BitSet final
{
   public:
    using Byte = uint8_t;
    using Word = uint64_t;

   private:
    static constexpr size_t BITS_PER_BYTE = 8;
    static constexpr size_t BITS_PER_WORD = sizeof(Word) * BITS_PER_BYTE;

    // Mask to extract the bit position within a word (0-63 for 64-bit words).
    static constexpr size_t WORD_MASK = BITS_PER_WORD - 1;

    // Shift amount to divide by BITS_PER_WORD (log2(64) = 6).
    static constexpr size_t WORD_SHIFT = 6;

    size_t m_size = 0;
    size_t m_wordCount = 0;
    ContiguousAllocatorBase<Word> m_allocator;

   public:
    /**
     * @brief Constructs a BitSet with the specified number of bits.
     *
     * @param _size The number of bits in the BitSet.
     * @throws std::invalid_argument if _size is zero.
     */
    inline explicit BitSet(size_t _size)
        : m_size(_size),
          m_wordCount((_size + BITS_PER_WORD - 1) / BITS_PER_WORD),
          m_allocator(m_wordCount * sizeof(Word))
    {
        if (_size == 0) throw std::invalid_argument("Size must be greater than zero.");
    }

    inline BitSet(const BitSet& _other)
        : m_size(_other.m_size),
          m_wordCount(_other.m_wordCount),
          m_allocator(_other.m_wordCount * sizeof(Word))
    {
        if (m_wordCount > 0)
        {
            std::memcpy(m_allocator.getBuffer(), _other.m_allocator.getBuffer(),
                        m_wordCount * sizeof(Word));
        }
    }

    [[nodiscard]] inline BitSet& operator=(const BitSet& _other)
    {
        if (this == &_other) return *this;

        m_size = _other.m_size;
        m_wordCount = _other.m_wordCount;

        if (m_wordCount > 0)
        {
            m_allocator = ContiguousAllocatorBase<Word>(m_wordCount * sizeof(Word));
            std::memcpy(m_allocator.getBuffer(), _other.m_allocator.getBuffer(),
                        m_wordCount * sizeof(Word));
        }
        else
        {
            m_allocator = ContiguousAllocatorBase<Word>(sizeof(Word));
        }

        return *this;
    }

    inline BitSet(BitSet&& _other) noexcept
        : m_allocator(std::move(_other.m_allocator)),
          m_size(_other.m_size),
          m_wordCount(_other.m_wordCount)
    {
        _other.m_size = 0;
        _other.m_wordCount = 0;
    }

    [[nodiscard]] inline BitSet& operator=(BitSet&& _other) noexcept
    {
        if (this == &_other) return *this;

        m_allocator = std::move(_other.m_allocator);
        m_size = _other.m_size;
        m_wordCount = _other.m_wordCount;

        _other.m_size = 0;
        _other.m_wordCount = 0;

        return *this;
    }

   private:
    // Returns a pointer to the underlying word array.
    [[nodiscard]] inline Word* getWords() noexcept
    {
        return reinterpret_cast<Word*>(m_allocator.getBuffer());
    }

    // Returns a const pointer to the underlying word array.
    [[nodiscard]] inline const Word* getWords() const noexcept
    {
        return reinterpret_cast<const Word*>(m_allocator.getBuffer());
    }

   public:
    /**
     * @brief Sets the bit at the specified index to 1 (true).
     *
     * @param _index The index of the bit to set.
     */
    inline void setBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        getWords()[wordIndex] |= (Word(1) << bitIndex);
    }

    /**
     * @brief Clears the bit at the specified index (sets it to 0).
     *
     * @param _index The index of the bit to clear.
     */
    inline void clearBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        getWords()[wordIndex] &= ~(Word(1) << bitIndex);
    }

    /**
     * @brief Tests if the bit at the specified index is set (1) or not (0).
     *
     * @param _index The index of the bit to test.
     * @return true if the bit is set, false otherwise.
     */
    [[nodiscard]] inline bool testBit(size_t _index) const noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        return (getWords()[wordIndex] & (Word(1) << bitIndex)) != 0;
    }

    /**
     * @brief Flips the bit at the specified index (0 to 1 or 1 to 0).
     *
     * @param _index The index of the bit to flip.
     */
    inline void flipBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        getWords()[wordIndex] ^= (Word(1) << bitIndex);
    }

    /**
     * @brief Finds the index of the first set bit (1) in the BitSet.
     *
     * @return size_t The index of the first set bit found, or size() if none is found.
     */
    [[nodiscard]] inline size_t findFirstSet() const noexcept
    {
        const Word* words = getWords();

        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (words[i] != 0) return i * BITS_PER_WORD + std::countr_zero(words[i]);
        }

        return m_size;
    }

    /**
     * @brief Finds the index of the first set bit (1) starting from a given index.
     *
     * @param _startIndex The index to start searching from.
     * @return size_t The index of the first set bit found, or size() if none is found.
     */
    [[nodiscard]] inline size_t findFirstSetFrom(size_t _startIndex) const noexcept
    {
        if (_startIndex >= m_size) return m_size;

        const Word* words = getWords();
        const size_t startWord = _startIndex >> WORD_SHIFT;
        const size_t startBit = _startIndex & WORD_MASK;

        Word firstWord = words[startWord];
        // Mask out bits before startBit
        if (startBit > 0) firstWord &= ~((Word(1) << startBit) - 1);

        if (firstWord != 0)
        {
            const size_t result = startWord * BITS_PER_WORD + std::countr_zero(firstWord);
            return result < m_size ? result : m_size;
        }

        // Check remaining words
        for (size_t i = startWord + 1; i < m_wordCount; ++i)
        {
            if (words[i] != 0)
            {
                const size_t result = i * BITS_PER_WORD + std::countr_zero(words[i]);
                return result < m_size ? result : m_size;
            }
        }

        return m_size;
    }

    /**
     * @brief Finds the index of the first clear bit (0) in the BitSet.
     *
     * @return size_t The index of the first clear bit, or size() if none is found.
     */
    [[nodiscard]] inline size_t findFirstClear() const noexcept
    {
        const Word* words = getWords();

        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (words[i] != ~Word(0))
            {
                const size_t result = i * BITS_PER_WORD + std::countr_one(words[i]);
                return result < m_size ? result : m_size;
            }
        }

        return m_size;
    }

    /**
     * @brief Finds the index of the first clear bit (0) starting from a given index.
     *
     * @param _startIndex The index to start searching from.
     * @return size_t The index of the first clear bit found, or size() if none is found.
     */
    [[nodiscard]] inline size_t findFirstClearFrom(size_t _startIndex) const noexcept
    {
        if (_startIndex >= m_size) return m_size;

        const Word* words = getWords();
        const size_t startWord = _startIndex >> WORD_SHIFT;
        const size_t startBit = _startIndex & WORD_MASK;

        Word firstWord = words[startWord];
        // Mask out bits before startBit by setting them to 1
        if (startBit > 0) firstWord |= ((Word(1) << startBit) - 1);

        if (firstWord != ~Word(0))
        {
            const size_t result = startWord * BITS_PER_WORD + std::countr_one(firstWord);
            return result < m_size ? result : m_size;
        }

        // Check remaining words
        for (size_t i = startWord + 1; i < m_wordCount; ++i)
        {
            if (words[i] != ~Word(0))
            {
                const size_t result = i * BITS_PER_WORD + std::countr_one(words[i]);
                return result < m_size ? result : m_size;
            }
        }

        return m_size;
    }

    /**
     * @brief Counts the number of set bits (1s) in the BitSet.
     */
    [[nodiscard]] inline size_t popcount() const noexcept
    {
        const Word* words = getWords();
        size_t count = 0;

        for (size_t i = 0; i < m_wordCount; ++i)
        {
            count += std::popcount(words[i]);
        }

        // Handle potential padding bits in the last word
        if (m_size % BITS_PER_WORD != 0)
        {
            const size_t paddingBits = BITS_PER_WORD - (m_size % BITS_PER_WORD);
            const Word lastWordMask = ~Word(0) >> paddingBits;
            const size_t lastWordPopcount = std::popcount(words[m_wordCount - 1] & lastWordMask);
            count = count - std::popcount(words[m_wordCount - 1]) + lastWordPopcount;
        }

        return count;
    }

    /**
     * @brief Set the entire BitSet to 1s (true).
     */
    inline void setAll() noexcept
    {
        std::memset(m_allocator.getBuffer(), 0xFF, m_wordCount * sizeof(Word));

        // Clear padding bits in the last word
        if (m_size % BITS_PER_WORD != 0)
        {
            const size_t paddingBits = BITS_PER_WORD - (m_size % BITS_PER_WORD);
            const Word mask = ~Word(0) >> paddingBits;
            getWords()[m_wordCount - 1] &= mask;
        }
    }

    /**
     * @brief Clear the entire BitSet to 0s (false).
     */
    inline void clearAll() noexcept
    {
        std::memset(m_allocator.getBuffer(), 0, m_wordCount * sizeof(Word));
    }

    // Returns the number of bits in the BitSet
    [[nodiscard]] inline size_t size() const noexcept { return m_size; }

    // Checks if the BitSet is empty (all bits are 0)
    [[nodiscard]] inline bool empty() const noexcept
    {
        const Word* words = getWords();

        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (words[i] != 0) return false;
        }

        return true;
    }

    // Checks if the BitSet is not empty (at least one bit is 1).
    [[nodiscard]] inline bool any() const noexcept { return !empty(); }

    // Checks if the BitSet has no bits set (all bits are 0).
    [[nodiscard]] inline bool none() const noexcept { return empty(); }

    // Alias for popcount to match common terminology.
    [[nodiscard]] inline size_t count() const noexcept { return popcount(); }

    // Returns a pointer to the underlying data (array of Words).
    [[nodiscard]] inline const Word* data() const noexcept { return getWords(); }

    // Returns the number of words used to store the bits.
    [[nodiscard]] inline size_t wordCount() const noexcept { return m_wordCount; }

    [[nodiscard]] bool operator==(const BitSet& _other) const noexcept
    {
        if (m_size != _other.m_size) return false;

        Word* words = reinterpret_cast<Word*>(m_allocator.getBuffer());
        Word* otherWords = reinterpret_cast<Word*>(_other.m_allocator.getBuffer());

        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (words[i] != otherWords[i]) return false;
        }

        return true;
    }

    [[nodiscard]] bool operator!=(const BitSet& _other) const noexcept
    {
        return !(*this == _other);
    }

    /**
     * @brief Bitwise AND operation between two BitSets.
     *
     * @param _other The other BitSet to perform the AND operation with.
     * @return BitSet The result of the bitwise AND operation.
     */
    [[nodiscard]] inline BitSet operator&(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        Word* resultWords = result.getWords();
        const Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) resultWords[i] = words[i] & otherWords[i];

        return result;
    }

    /**
     * @brief Bitwise OR operation between two BitSets.
     *
     * @param _other The other BitSet to perform the OR operation with.
     * @return BitSet The result of the bitwise OR operation.
     */
    [[nodiscard]] inline BitSet operator|(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        Word* resultWords = result.getWords();
        const Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) resultWords[i] = words[i] | otherWords[i];

        return result;
    }

    /**
     * @brief Bitwise XOR operation between two BitSets.
     *
     * @param _other The other BitSet to perform the XOR operation with.
     * @return BitSet The result of the bitwise XOR operation.
     */
    [[nodiscard]] inline BitSet operator^(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        Word* resultWords = result.getWords();
        const Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) resultWords[i] = words[i] ^ otherWords[i];

        return result;
    }

    /**
     * @brief Compound assignment bitwise AND operation.
     *
     * @param _other The other BitSet to perform the AND operation with.
     * @return BitSet& Reference to this BitSet after the operation.
     */
    [[nodiscard]] inline BitSet& operator&=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) words[i] &= otherWords[i];

        return *this;
    }

    /**
     * @brief Compound assignment bitwise OR operation.
     *
     * @param _other The other BitSet to perform the OR operation with.
     * @return BitSet& Reference to this BitSet after the operation.
     */
    [[nodiscard]] inline BitSet& operator|=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) words[i] |= otherWords[i];

        return *this;
    }

    /**
     * @brief Compound assignment bitwise XOR operation.
     *
     * @param _other The other BitSet to perform the XOR operation with.
     * @return BitSet& Reference to this BitSet after the operation.
     */
    [[nodiscard]] inline BitSet& operator^=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        Word* words = getWords();
        const Word* otherWords = _other.getWords();

        for (size_t i = 0; i < m_wordCount; ++i) words[i] ^= otherWords[i];

        return *this;
    }
};

} // namespace pieces
