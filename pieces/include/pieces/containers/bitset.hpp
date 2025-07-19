#pragma once

#include <bit>
#include <cstring>
#include <cassert>
#include <stdexcept>

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

    Word* m_words = nullptr;
    size_t m_size = 0;
    size_t m_wordCount = 0;

   public:
    inline explicit BitSet(size_t _size)
        : m_size(_size), m_wordCount((_size + BITS_PER_WORD - 1) / BITS_PER_WORD)
    {
        if (_size == 0) throw std::invalid_argument("Size must be greater than zero.");

        m_words = new Word[m_wordCount]();
    }

    inline ~BitSet() { delete[] m_words; }

    inline BitSet(const BitSet& _other) : m_size(_other.m_size), m_wordCount(_other.m_wordCount)
    {
        if (m_wordCount < 0) return;

        m_words = new Word[m_wordCount];
        std::memcpy(m_words, _other.m_words, m_wordCount * sizeof(Word));
    }

    [[nodiscard]] inline BitSet& operator=(const BitSet& _other)
    {
        if (this == &_other) return *this;

        delete[] m_words;

        m_size = _other.m_size;
        m_wordCount = _other.m_wordCount;

        if (m_wordCount > 0)
        {
            m_words = new Word[m_wordCount];
            std::memcpy(m_words, _other.m_words, m_wordCount * sizeof(Word));
        }
        else
        {
            m_words = nullptr;
        }

        return *this;
    }

    inline BitSet(BitSet&& _other) noexcept
        : m_words(_other.m_words), m_size(_other.m_size), m_wordCount(_other.m_wordCount)
    {
        _other.m_words = nullptr;
        _other.m_size = 0;
        _other.m_wordCount = 0;
    }

    [[nodiscard]] inline BitSet& operator=(BitSet&& _other) noexcept
    {
        if (this == &_other) return *this;

        delete[] m_words;

        m_words = _other.m_words;
        m_size = _other.m_size;
        m_wordCount = _other.m_wordCount;

        _other.m_words = nullptr;
        _other.m_size = 0;
        _other.m_wordCount = 0;

        return *this;
    }

   public:
    inline void setBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        m_words[wordIndex] |= (Word(1) << bitIndex);
    }

    inline void clearBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        m_words[wordIndex] &= ~(Word(1) << bitIndex);
    }

    [[nodiscard]] inline bool testBit(size_t _index) const noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        return (m_words[wordIndex] & (Word(1) << bitIndex)) != 0;
    }

    inline void flipBit(size_t _index) noexcept
    {
        assert(_index < m_size && "Index out of bounds");
        const size_t wordIndex = _index >> WORD_SHIFT;
        const size_t bitIndex = _index & WORD_MASK;
        m_words[wordIndex] ^= (Word(1) << bitIndex);
    }

    [[nodiscard]] inline size_t findFirstSet() const noexcept
    {
        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (m_words[i] != 0) return i * BITS_PER_WORD + std::countr_zero(m_words[i]);
        }

        return m_size;
    }

    [[nodiscard]] inline size_t findFirstSetFrom(size_t _startIndex) const noexcept
    {
        if (_startIndex >= m_size) return m_size;

        const size_t startWord = _startIndex >> WORD_SHIFT;
        const size_t startBit = _startIndex & WORD_MASK;

        Word firstWord = m_words[startWord];
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
            if (m_words[i] != 0)
            {
                const size_t result = i * BITS_PER_WORD + std::countr_zero(m_words[i]);
                return result < m_size ? result : m_size;
            }
        }

        return m_size;
    }

    [[nodiscard]] inline size_t findFirstClear() const noexcept
    {
        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (m_words[i] != ~Word(0))
            {
                const size_t result = i * BITS_PER_WORD + std::countr_one(m_words[i]);
                return result < m_size ? result : m_size;
            }
        }

        return m_size;
    }

    [[nodiscard]] inline size_t popcount() const noexcept
    {
        size_t count = 0;
        for (size_t i = 0; i < m_wordCount; ++i)
        {
            count += std::popcount(m_words[i]);
        }

        // Handle potential padding bits in the last word
        if (m_size % BITS_PER_WORD != 0)
        {
            const size_t paddingBits = BITS_PER_WORD - (m_size % BITS_PER_WORD);
            const Word lastWordMask = ~Word(0) >> paddingBits;
            const size_t lastWordPopcount = std::popcount(m_words[m_wordCount - 1] & lastWordMask);
            count = count - std::popcount(m_words[m_wordCount - 1]) + lastWordPopcount;
        }

        return count;
    }

    inline void setAll() noexcept
    {
        std::memset(m_words, 0xFF, m_wordCount * sizeof(Word));

        // Clear padding bits in the last word
        if (m_size % BITS_PER_WORD != 0)
        {
            const size_t paddingBits = BITS_PER_WORD - (m_size % BITS_PER_WORD);
            const Word mask = ~Word(0) >> paddingBits;
            m_words[m_wordCount - 1] &= mask;
        }
    }

    inline void clearAll() noexcept { std::memset(m_words, 0, m_wordCount * sizeof(Word)); }

    [[nodiscard]] inline size_t size() const noexcept { return m_size; }

    [[nodiscard]] inline bool empty() const noexcept
    {
        for (size_t i = 0; i < m_wordCount; ++i)
        {
            if (m_words[i] != 0) return false;
        }

        return true;
    }

    [[nodiscard]] inline bool any() const noexcept { return !empty(); }

    [[nodiscard]] inline bool none() const noexcept { return empty(); }

    [[nodiscard]] inline size_t count() const noexcept { return popcount(); }

    [[nodiscard]] inline BitSet operator&(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        for (size_t i = 0; i < m_wordCount; ++i) result.m_words[i] = m_words[i] & _other.m_words[i];

        return result;
    }

    [[nodiscard]] inline BitSet operator|(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        for (size_t i = 0; i < m_wordCount; ++i) result.m_words[i] = m_words[i] | _other.m_words[i];

        return result;
    }

    [[nodiscard]] inline BitSet operator^(const BitSet& _other) const
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        BitSet result(m_size);
        for (size_t i = 0; i < m_wordCount; ++i) result.m_words[i] = m_words[i] ^ _other.m_words[i];

        return result;
    }

    [[nodiscard]] inline BitSet& operator&=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        for (size_t i = 0; i < m_wordCount; ++i) m_words[i] &= _other.m_words[i];

        return *this;
    }

    [[nodiscard]] inline BitSet& operator|=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        for (size_t i = 0; i < m_wordCount; ++i) m_words[i] |= _other.m_words[i];

        return *this;
    }

    [[nodiscard]] inline BitSet& operator^=(const BitSet& _other)
    {
        if (m_size != _other.m_size)
        {
            throw std::invalid_argument("BitSet sizes must match for bitwise operations");
        }

        for (size_t i = 0; i < m_wordCount; ++i) m_words[i] ^= _other.m_words[i];

        return *this;
    }
};

} // namespace pieces
