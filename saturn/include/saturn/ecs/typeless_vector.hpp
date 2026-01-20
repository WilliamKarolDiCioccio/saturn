#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <utility>

#include <pieces/memory/contiguous_allocator.hpp>

namespace saturn
{
namespace ecs
{

/**
 * @brief A vector-like container for untyped, fixed-stride elements.
 * Mimics std::vector API closely, except it is untyped.
 *
 * Backed by ContiguousAllocatorBase<uint8_t>.
 */
class TypelessVector final
{
   public:
    using Byte = uint8_t;

   private:
    size_t m_stride;
    pieces::ContiguousAllocatorBase<Byte> m_allocator;
    size_t m_size;
    size_t m_capacity;

   public:
    /**
     * @brief Constructs a TypelessVector with the specified stride and initial capacity.
     *
     * @param _stride The size in bytes of each element (must be > 0).
     * @param _capacity The initial capacity of the vector (default is 1).
     * @throws std::invalid_argument if _stride is 0.
     */
    TypelessVector(size_t _stride, size_t _capacity = 1)
        : m_stride(_stride), m_allocator(_capacity * _stride), m_size(0), m_capacity(_capacity)
    {
        if (_stride == 0) throw std::invalid_argument("Stride must be > 0");
    }

    TypelessVector(TypelessVector&& _other) noexcept
        : m_stride(_other.m_stride),
          m_allocator(std::move(_other.m_allocator)),
          m_size(_other.m_size),
          m_capacity(_other.m_capacity)
    {
        _other.m_stride = 0;
        _other.m_size = 0;
        _other.m_capacity = 0;
    }

    TypelessVector& operator=(TypelessVector&& _other) noexcept
    {
        if (this == &_other) return *this;

        m_stride = _other.m_stride;
        m_allocator = std::move(_other.m_allocator);
        m_size = _other.m_size;
        m_capacity = _other.m_capacity;

        _other.m_stride = 0;
        _other.m_size = 0;
        _other.m_capacity = 0;
    }

    TypelessVector(const TypelessVector&) = delete;
    TypelessVector& operator=(const TypelessVector&) = delete;

    Byte* at(size_t _idx)
    {
        if (_idx >= m_size) throw std::out_of_range("TypelessVector index out of range");
        return (*this)[_idx];
    }

    const Byte* at(size_t _idx) const
    {
        if (_idx >= m_size) throw std::out_of_range("TypelessVector index out of range");
        return (*this)[_idx];
    }

    Byte* data() { return m_allocator.getBuffer(); }
    const Byte* data() const { return m_allocator.getBuffer(); }

    void pushBack(const void* _src)
    {
        ensureCapacity(m_size + 1);
        Byte* dest = (*this)[m_size];
        std::memcpy(dest, _src, m_stride);
        ++m_size;
    }

    void popBack()
    {
        if (m_size == 0) throw std::runtime_error("TypelessVector::pop_back on empty vector");
        --m_size;
    }

    /**
     * @brief Appends multiple elements from a contiguous source buffer.
     *
     * @param _src Pointer to contiguous source data (count * stride bytes).
     * @param _count Number of elements to append.
     */
    void pushBackBulk(const void* _src, size_t _count)
    {
        if (_count == 0) return;

        ensureCapacity(m_size + _count);
        Byte* dest = (*this)[m_size];
        std::memcpy(dest, _src, _count * m_stride);
        m_size += _count;
    }

    /**
     * @brief Appends uninitialized slots to the vector.
     *
     * Caller is responsible for initializing the data via data() + offset.
     *
     * @param _count Number of uninitialized slots to append.
     * @return Pointer to the first uninitialized slot.
     */
    Byte* appendUninitialized(size_t _count)
    {
        if (_count == 0) return nullptr;

        ensureCapacity(m_size + _count);
        Byte* dest = (*this)[m_size];
        m_size += _count;
        return dest;
    }

    /**
     * @brief Erases multiple elements from the end of the vector.
     *
     * @param _count Number of elements to erase from the end.
     */
    void popBackBulk(size_t _count)
    {
        if (_count > m_size)
            throw std::runtime_error("TypelessVector::popBackBulk count exceeds size");
        m_size -= _count;
    }

    void erase(size_t _idx)
    {
        if (_idx >= m_size) throw std::out_of_range("TypelessVector::erase out of range");

        if (_idx != m_size - 1)
        {
            Byte* dest = (*this)[_idx];
            Byte* last = (*this)[m_size - 1];
            std::memcpy(dest, last, m_stride);
        }

        --m_size;
    }

    void swapElements(size_t _i, size_t _j)
    {
        if (_i == _j) return;

        Byte* a = m_allocator.getBuffer() + _i * m_stride;
        Byte* b = m_allocator.getBuffer() + _j * m_stride;

        std::unique_ptr<Byte[]> tmp(new Byte[m_stride]);

        std::memcpy(tmp.get(), a, m_stride);
        std::memcpy(a, b, m_stride);
        std::memcpy(b, tmp.get(), m_stride);
    }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }
    size_t stride() const { return m_stride; }

    size_t memoryUsageInBytes() const { return sizeof(*this) + m_capacity * m_stride; }

    void reserve(size_t _capacity)
    {
        if (_capacity <= m_capacity) return;

        pieces::ContiguousAllocatorBase<Byte> newAlloc(_capacity * m_stride);

        if (m_size > 0)
        {
            std::memcpy(newAlloc.getBuffer(), m_allocator.getBuffer(), m_size * m_stride);
        }

        m_allocator = std::move(newAlloc);
        m_capacity = _capacity;
    }

    void resize(size_t _size)
    {
        if (_size > m_capacity) reserve(_size);
        m_size = _size;
    }

    void clear() { m_size = 0; }

    void shrinkToFit()
    {
        if (m_size < m_capacity)
        {
            pieces::ContiguousAllocatorBase<Byte> newAlloc(m_size * m_stride);

            if (m_size > 0)
            {
                std::memcpy(newAlloc.getBuffer(), m_allocator.getBuffer(), m_size * m_stride);
            }

            m_allocator = std::move(newAlloc);
            m_capacity = m_size;
        }
    }

    Byte* operator[](size_t _idx) { return m_allocator.getBuffer() + _idx * m_stride; }

    const Byte* operator[](size_t _idx) const { return m_allocator.getBuffer() + _idx * m_stride; }

   public:
    struct Iterator
    {
        Byte* ptr;
        size_t stride;

        Iterator(Byte* _ptr, size_t _size) : ptr(_ptr), stride(_size) {}

        Iterator& operator++()
        {
            ptr += stride;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& _other) const { return ptr == _other.ptr; }
        bool operator!=(const Iterator& _other) const { return ptr != _other.ptr; }

        Byte* operator*() const { return ptr; }
    };

    Iterator begin() { return Iterator(m_allocator.getBuffer(), m_stride); }
    Iterator end() { return Iterator(m_allocator.getBuffer() + m_size * m_stride, m_stride); }

    Iterator begin() const { return Iterator(m_allocator.getBuffer(), m_stride); }
    Iterator end() const { return Iterator(m_allocator.getBuffer() + m_size * m_stride, m_stride); }

   private:
    void ensureCapacity(size_t _required)
    {
        if (_required < m_capacity) return;

        size_t newCap = std::max(_required, m_capacity == 0 ? size_t(1) : m_capacity * 2);
        reserve(newCap);
    }
};

} // namespace ecs
} // namespace saturn
