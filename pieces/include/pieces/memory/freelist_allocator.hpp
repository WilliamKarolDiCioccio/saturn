#pragma once

#include <new>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <cstring>
#include <cstddef>

#include "pieces/core/templates.hpp"

namespace pieces
{

/**
 * @brief An enum class to define the allocation strategy policy for free-list allocators.
 */
enum class FreeListAllocatorPolicy : uint8_t
{
    first_fit, // Fast: allocates first block that fits (early exit)
    best_fit,  // Memory efficient: allocates smallest block that fits (full scan)
    worst_fit, // Fragment reduction: allocates largest block (full scan)
};

/**
 * @brief An enum class to define the coalescing strategy for free-list allocators.
 */
enum class CoalescingPolicy : uint8_t
{
    immediate, // Merge adjacent free blocks on every deallocate (low fragmentation, higher cost)
    deferred,  // Merge only when allocation fails (better performance, on-demand defragmentation)
    none,      // Never merge blocks (fastest, highest fragmentation)
};

/**
 * @brief A type-erased free-list allocator for variable-sized memory allocations.
 *
 * Unlike other allocators in Pieces, FreeListAllocator is NOT templated on a type parameter.
 * It manages variable-sized byte allocations and can hold many different types of data,
 * making efficient use of space through block coalescing and splitting.
 *
 * The allocator uses an in-place linked list of free blocks with embedded headers.
 * All allocations are aligned to std::max_align_t for compatibility with any type.
 *
 * @tparam Policy The allocation strategy (default: first_fit)
 * @tparam CoalescePolicy The coalescing strategy (default: deferred)
 *
 * @note This allocator provides raw memory allocation (void*) without type information.
 * @note Users must manage object construction/destruction separately.
 * @note All allocations are aligned to std::max_align_t.
 */
template <FreeListAllocatorPolicy Policy = FreeListAllocatorPolicy::first_fit,
          CoalescingPolicy CoalescePolicy = CoalescingPolicy::deferred>
class FreeListAllocator final : public NonCopyable<FreeListAllocator<Policy, CoalescePolicy>>
{
   public:
    using Byte = uint8_t;

   private:
    /**
     * @brief Block header structure for the in-place linked list.
     *
     * Each block (free or allocated) begins with this header.
     * Free blocks are linked together via the next pointer.
     */
    struct BlockHeader
    {
        size_t size;       // Total block size including this header
        BlockHeader* next; // Next free block in the free list (nullptr if allocated/last)
        bool isFree;       // true if block is free, false if allocated

        // User data follows immediately after header, aligned to max_align_t
        alignas(std::max_align_t) Byte userData[];
    };

    static constexpr size_t SIZEOF_HEADER = sizeof(BlockHeader);
    static constexpr size_t MIN_BLOCK_SIZE = SIZEOF_HEADER + 1;
    static constexpr size_t ALIGNMENT = alignof(std::max_align_t);

    Byte* m_bufferBytes;         // Raw buffer managed by allocator
    BlockHeader* m_freeListHead; // Head of free list (linked list of free blocks)
    size_t m_capacityInBytes;    // Total buffer size in bytes
    size_t m_usedInBytes;        // Currently allocated bytes (excluding headers)
    size_t m_totalOverhead;      // Total header overhead in bytes

    // Helper methods
    size_t alignSize(size_t _size) const noexcept;
    void splitBlock(BlockHeader* _block, size_t _requiredSize);
    void unlinkFromFreeList(BlockHeader* _prev, BlockHeader* _current);
    void insertIntoFreeList(BlockHeader* _block);
    void coalesceAdjacent(BlockHeader* _block);
    void coalesceAll();
    void rebuildFreeList();
    void* allocateInternal(size_t _sizeInBytes, bool _retryAfterCoalesce);

   public:
    /**
     * @brief Constructs a FreeListAllocator with a given capacity in bytes.
     *
     * @param _capacityInBytes The total buffer size in bytes.
     *
     * @throws std::invalid_argument if _capacityInBytes is too small to hold minimum block.
     */
    explicit FreeListAllocator(size_t _capacityInBytes);

    /**
     * @brief Destructor that frees the internal buffer.
     */
    ~FreeListAllocator();

    /**
     * @brief Move constructor.
     */
    FreeListAllocator(FreeListAllocator&& _other) noexcept;

    /**
     * @brief Move assignment operator.
     */
    FreeListAllocator& operator=(FreeListAllocator&& _other) noexcept;

   public:
    /**
     * @brief Allocates memory for the given size in bytes.
     *
     * @param _sizeInBytes The number of bytes to allocate.
     * @return Pointer to allocated memory aligned to std::max_align_t, or nullptr if allocation
     * fails.
     *
     * @note Size is automatically aligned to std::max_align_t boundary.
     * @note With deferred coalescing, may trigger coalesceAll() on allocation failure.
     */
    [[nodiscard]] void* allocate(size_t _sizeInBytes);

    /**
     * @brief Deallocates previously allocated memory.
     *
     * @param _ptr Pointer to memory to deallocate.
     * @param _sizeInBytes The size that was originally allocated.
     *
     * @note Pointer must have been returned by allocate().
     * @note Size must match the original allocation size.
     * @note With immediate coalescing, merges adjacent free blocks.
     * @note Safe to call with nullptr (no-op).
     * @note Double-free is detected and ignored.
     */
    void deallocate(void* _ptr, size_t _sizeInBytes) noexcept;

    /**
     * @brief Resets the allocator to its initial state.
     *
     * All allocations are invalidated. Buffer is zeroed and reset to single free block.
     */
    void reset();

    /**
     * @brief Returns the total usable capacity in bytes (excluding minimum header overhead).
     *
     * @return Total capacity in bytes.
     */
    [[nodiscard]] size_t capacity() const noexcept;

    /**
     * @brief Returns the currently allocated bytes (excluding headers).
     *
     * @return Used bytes.
     */
    [[nodiscard]] size_t used() const noexcept;

    /**
     * @brief Returns the available bytes for allocation.
     *
     * @return Available bytes (capacity - used).
     *
     * @note Due to fragmentation, available() bytes may not be allocatable as single block.
     */
    [[nodiscard]] size_t available() const noexcept;

    /**
     * @brief Checks if the allocator owns the given pointer.
     *
     * @param _ptr Pointer to check.
     * @return true if pointer was allocated by this allocator and is currently allocated.
     */
    [[nodiscard]] bool owns(void* _ptr) const noexcept;

    /**
     * @brief Returns the raw buffer pointer.
     *
     * @return Pointer to internal buffer.
     */
    [[nodiscard]] Byte* getBuffer() const noexcept;

    /**
     * @brief Returns the number of free blocks in the free list.
     *
     * @return Free block count.
     *
     * @note High count indicates fragmentation.
     */
    [[nodiscard]] size_t getFreeBlockCount() const noexcept;

    /**
     * @brief Returns the size of the largest free block.
     *
     * @return Largest free block size in bytes (excluding header).
     */
    [[nodiscard]] size_t getLargestFreeBlock() const noexcept;

    /**
     * @brief Returns a fragmentation ratio metric.
     *
     * @return Ratio between 0.0 (no fragmentation) and 1.0 (high fragmentation).
     *
     * @note Calculated as: 1.0 - (largest_free_block / total_free_bytes)
     */
    [[nodiscard]] float getFragmentationRatio() const noexcept;

    /**
     * @brief Equality operator.
     */
    [[nodiscard]] bool operator==(const FreeListAllocator& _other) const noexcept;

    /**
     * @brief Inequality operator.
     */
    [[nodiscard]] bool operator!=(const FreeListAllocator& _other) const noexcept;
};

/**
 * @brief Type alias for a first-fit free-list allocator with deferred coalescing.
 */
template <CoalescingPolicy CoalescePolicy = CoalescingPolicy::deferred>
using FirstFitAllocator = FreeListAllocator<FreeListAllocatorPolicy::first_fit, CoalescePolicy>;

/**
 * @brief Type alias for a best-fit free-list allocator with deferred coalescing.
 */
template <CoalescingPolicy CoalescePolicy = CoalescingPolicy::deferred>
using BestFitAllocator = FreeListAllocator<FreeListAllocatorPolicy::best_fit, CoalescePolicy>;

/**
 * @brief Type alias for a worst-fit free-list allocator with deferred coalescing.
 */
template <CoalescingPolicy CoalescePolicy = CoalescingPolicy::deferred>
using WorstFitAllocator = FreeListAllocator<FreeListAllocatorPolicy::worst_fit, CoalescePolicy>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
FreeListAllocator<Policy, CoalescePolicy>::FreeListAllocator(size_t _capacityInBytes)
    : m_bufferBytes(nullptr),
      m_freeListHead(nullptr),
      m_capacityInBytes(_capacityInBytes),
      m_usedInBytes(0),
      m_totalOverhead(SIZEOF_HEADER)
{
    if (_capacityInBytes < MIN_BLOCK_SIZE)
    {
        throw std::invalid_argument("Capacity too small for allocator");
    }

    // Allocate aligned buffer
    m_bufferBytes =
        static_cast<Byte*>(::operator new(_capacityInBytes, std::align_val_t{ALIGNMENT}));

    std::memset(m_bufferBytes, 0, _capacityInBytes);

    // Initialize single free block spanning entire buffer
    m_freeListHead = reinterpret_cast<BlockHeader*>(m_bufferBytes);
    m_freeListHead->size = _capacityInBytes;
    m_freeListHead->isFree = true;
    m_freeListHead->next = nullptr;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
FreeListAllocator<Policy, CoalescePolicy>::~FreeListAllocator()
{
    ::operator delete(m_bufferBytes, std::align_val_t{ALIGNMENT});
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
FreeListAllocator<Policy, CoalescePolicy>::FreeListAllocator(FreeListAllocator&& _other) noexcept
    : m_bufferBytes(_other.m_bufferBytes),
      m_freeListHead(_other.m_freeListHead),
      m_capacityInBytes(_other.m_capacityInBytes),
      m_usedInBytes(_other.m_usedInBytes),
      m_totalOverhead(_other.m_totalOverhead)
{
    _other.m_bufferBytes = nullptr;
    _other.m_freeListHead = nullptr;
    _other.m_capacityInBytes = 0;
    _other.m_usedInBytes = 0;
    _other.m_totalOverhead = 0;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
FreeListAllocator<Policy, CoalescePolicy>& FreeListAllocator<Policy, CoalescePolicy>::operator=(
    FreeListAllocator&& _other) noexcept
{
    if (this == &_other) return *this;

    ::operator delete(m_bufferBytes, std::align_val_t{ALIGNMENT});

    m_bufferBytes = _other.m_bufferBytes;
    m_freeListHead = _other.m_freeListHead;
    m_capacityInBytes = _other.m_capacityInBytes;
    m_usedInBytes = _other.m_usedInBytes;
    m_totalOverhead = _other.m_totalOverhead;

    _other.m_bufferBytes = nullptr;
    _other.m_freeListHead = nullptr;
    _other.m_capacityInBytes = 0;
    _other.m_usedInBytes = 0;
    _other.m_totalOverhead = 0;

    return *this;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::alignSize(size_t _size) const noexcept
{
    return (_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void* FreeListAllocator<Policy, CoalescePolicy>::allocate(size_t _sizeInBytes)
{
    return allocateInternal(_sizeInBytes, false);
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void* FreeListAllocator<Policy, CoalescePolicy>::allocateInternal(size_t _sizeInBytes,
                                                                  bool _retryAfterCoalesce)
{
    if (_sizeInBytes == 0) return nullptr;

    size_t alignedSize = alignSize(_sizeInBytes);
    size_t totalSize = SIZEOF_HEADER + alignedSize;

    BlockHeader* prev = nullptr;
    BlockHeader* current = m_freeListHead;
    BlockHeader* bestFit = nullptr;
    BlockHeader* bestFitPrev = nullptr;

    if constexpr (Policy == FreeListAllocatorPolicy::first_fit)
    {
        // First-fit: find first block that fits
        while (current != nullptr)
        {
            if (current->isFree && current->size >= totalSize)
            {
                splitBlock(current, totalSize);
                unlinkFromFreeList(prev, current);
                current->isFree = false;
                m_usedInBytes += alignedSize;
                return current->userData;
            }
            prev = current;
            current = current->next;
        }
    }
    else if constexpr (Policy == FreeListAllocatorPolicy::best_fit)
    {
        // Best-fit: find smallest block that fits
        size_t bestSize = SIZE_MAX;
        while (current != nullptr)
        {
            if (current->isFree && current->size >= totalSize && current->size < bestSize)
            {
                bestFit = current;
                bestFitPrev = prev;
                bestSize = current->size;
            }
            prev = current;
            current = current->next;
        }

        if (bestFit != nullptr)
        {
            splitBlock(bestFit, totalSize);
            unlinkFromFreeList(bestFitPrev, bestFit);
            bestFit->isFree = false;
            m_usedInBytes += alignedSize;
            return bestFit->userData;
        }
    }
    else if constexpr (Policy == FreeListAllocatorPolicy::worst_fit)
    {
        // Worst-fit: find largest block
        size_t worstSize = 0;
        while (current != nullptr)
        {
            if (current->isFree && current->size >= totalSize && current->size > worstSize)
            {
                bestFit = current;
                bestFitPrev = prev;
                worstSize = current->size;
            }
            prev = current;
            current = current->next;
        }

        if (bestFit != nullptr)
        {
            splitBlock(bestFit, totalSize);
            unlinkFromFreeList(bestFitPrev, bestFit);
            bestFit->isFree = false;
            m_usedInBytes += alignedSize;
            return bestFit->userData;
        }
    }

    // Allocation failed - try deferred coalescing
    if constexpr (CoalescePolicy == CoalescingPolicy::deferred)
    {
        if (!_retryAfterCoalesce)
        {
            coalesceAll();
            return allocateInternal(_sizeInBytes, true);
        }
    }

    return nullptr;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::splitBlock(BlockHeader* _block,
                                                           size_t _requiredSize)
{
    size_t remaining = _block->size - _requiredSize;

    // Only split if remainder can hold at least MIN_BLOCK_SIZE
    if (remaining >= MIN_BLOCK_SIZE)
    {
        Byte* newBlockAddr = reinterpret_cast<Byte*>(_block) + _requiredSize;
        BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(newBlockAddr);

        newBlock->size = remaining;
        newBlock->isFree = true;
        newBlock->next = _block->next;

        _block->size = _requiredSize;
        _block->next = newBlock;

        m_totalOverhead += SIZEOF_HEADER;
    }
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::unlinkFromFreeList(BlockHeader* _prev,
                                                                   BlockHeader* _current)
{
    if (_prev == nullptr)
    {
        // Unlinking head of free list
        m_freeListHead = _current->next;
    }
    else
    {
        _prev->next = _current->next;
    }
    _current->next = nullptr;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::deallocate(void* _ptr, size_t _sizeInBytes) noexcept
{
    if (!_ptr || !owns(_ptr)) return;

    // Get block header (userData is at offset within BlockHeader)
    Byte* bytePtr = reinterpret_cast<Byte*>(_ptr);
    BlockHeader* block = reinterpret_cast<BlockHeader*>(bytePtr - offsetof(BlockHeader, userData));

    if (block->isFree) return; // Double-free protection

    block->isFree = true;

    size_t alignedSize = alignSize(_sizeInBytes);
    m_usedInBytes -= alignedSize;

    // Insert into free list
    insertIntoFreeList(block);

    // Coalesce if policy is immediate
    if constexpr (CoalescePolicy == CoalescingPolicy::immediate)
    {
        coalesceAdjacent(block);
    }
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::insertIntoFreeList(BlockHeader* _block)
{
    // Insert in address-ordered position for efficient coalescing
    if (m_freeListHead == nullptr || _block < m_freeListHead)
    {
        _block->next = m_freeListHead;
        m_freeListHead = _block;
        return;
    }

    BlockHeader* current = m_freeListHead;
    while (current->next != nullptr && current->next < _block)
    {
        current = current->next;
    }

    _block->next = current->next;
    current->next = _block;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::coalesceAdjacent(BlockHeader* _block)
{
    // Try to merge with next block
    Byte* nextAddr = reinterpret_cast<Byte*>(_block) + _block->size;
    if (nextAddr < m_bufferBytes + m_capacityInBytes)
    {
        BlockHeader* next = reinterpret_cast<BlockHeader*>(nextAddr);
        if (next->isFree)
        {
            // Merge with next
            _block->size += next->size;
            _block->next = next->next;
            m_totalOverhead -= SIZEOF_HEADER;
        }
    }

    // Try to merge with previous block
    BlockHeader* prev = nullptr;
    BlockHeader* current = m_freeListHead;
    while (current != nullptr && current != _block)
    {
        prev = current;
        current = current->next;
    }

    if (prev != nullptr)
    {
        Byte* prevEndAddr = reinterpret_cast<Byte*>(prev) + prev->size;
        if (prevEndAddr == reinterpret_cast<Byte*>(_block))
        {
            // Merge with previous
            prev->size += _block->size;
            prev->next = _block->next;
            m_totalOverhead -= SIZEOF_HEADER;
        }
    }
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::coalesceAll()
{
    BlockHeader* current = reinterpret_cast<BlockHeader*>(m_bufferBytes);

    while (reinterpret_cast<Byte*>(current) < m_bufferBytes + m_capacityInBytes)
    {
        if (current->isFree)
        {
            // Merge all consecutive free blocks
            Byte* nextAddr = reinterpret_cast<Byte*>(current) + current->size;

            while (nextAddr < m_bufferBytes + m_capacityInBytes)
            {
                BlockHeader* next = reinterpret_cast<BlockHeader*>(nextAddr);
                if (!next->isFree) break;

                current->size += next->size;
                m_totalOverhead -= SIZEOF_HEADER;
                nextAddr = reinterpret_cast<Byte*>(current) + current->size;
            }
        }

        current = reinterpret_cast<BlockHeader*>(reinterpret_cast<Byte*>(current) + current->size);
    }

    // Rebuild free list after coalescing
    rebuildFreeList();
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::rebuildFreeList()
{
    m_freeListHead = nullptr;
    BlockHeader* lastFree = nullptr;

    BlockHeader* current = reinterpret_cast<BlockHeader*>(m_bufferBytes);

    while (reinterpret_cast<Byte*>(current) < m_bufferBytes + m_capacityInBytes)
    {
        if (current->isFree)
        {
            if (m_freeListHead == nullptr)
            {
                m_freeListHead = current;
                lastFree = current;
            }
            else
            {
                lastFree->next = current;
                lastFree = current;
            }
            current->next = nullptr;
        }

        current = reinterpret_cast<BlockHeader*>(reinterpret_cast<Byte*>(current) + current->size);
    }
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
void FreeListAllocator<Policy, CoalescePolicy>::reset()
{
    std::memset(m_bufferBytes, 0, m_capacityInBytes);

    m_freeListHead = reinterpret_cast<BlockHeader*>(m_bufferBytes);
    m_freeListHead->size = m_capacityInBytes;
    m_freeListHead->isFree = true;
    m_freeListHead->next = nullptr;

    m_usedInBytes = 0;
    m_totalOverhead = SIZEOF_HEADER;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::capacity() const noexcept
{
    return m_capacityInBytes > SIZEOF_HEADER ? m_capacityInBytes - SIZEOF_HEADER : 0;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::used() const noexcept
{
    return m_usedInBytes;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::available() const noexcept
{
    return capacity() - used();
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
bool FreeListAllocator<Policy, CoalescePolicy>::owns(void* _ptr) const noexcept
{
    if (!_ptr || !m_bufferBytes) return false;

    uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(_ptr);
    uintptr_t bufferStart = reinterpret_cast<uintptr_t>(m_bufferBytes);
    uintptr_t bufferEnd = bufferStart + m_capacityInBytes;

    if (ptrAddr < bufferStart + SIZEOF_HEADER || ptrAddr >= bufferEnd)
    {
        return false;
    }

    // Verify this points to userData of a valid allocated block
    Byte* bytePtr = reinterpret_cast<Byte*>(_ptr);
    BlockHeader* header = reinterpret_cast<BlockHeader*>(bytePtr - offsetof(BlockHeader, userData));

    return !header->isFree;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
typename FreeListAllocator<Policy, CoalescePolicy>::Byte*
FreeListAllocator<Policy, CoalescePolicy>::getBuffer() const noexcept
{
    return m_bufferBytes;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::getFreeBlockCount() const noexcept
{
    size_t count = 0;
    BlockHeader* current = m_freeListHead;

    while (current != nullptr)
    {
        count++;
        current = current->next;
    }

    return count;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
size_t FreeListAllocator<Policy, CoalescePolicy>::getLargestFreeBlock() const noexcept
{
    size_t largest = 0;
    BlockHeader* current = m_freeListHead;

    while (current != nullptr)
    {
        if (current->size > largest)
        {
            largest = current->size;
        }
        current = current->next;
    }

    return largest > SIZEOF_HEADER ? largest - SIZEOF_HEADER : 0;
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
float FreeListAllocator<Policy, CoalescePolicy>::getFragmentationRatio() const noexcept
{
    size_t totalFree = available();
    if (totalFree == 0) return 0.0f;

    size_t largest = getLargestFreeBlock();
    return 1.0f - (static_cast<float>(largest) / static_cast<float>(totalFree));
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
bool FreeListAllocator<Policy, CoalescePolicy>::operator==(
    const FreeListAllocator& _other) const noexcept
{
    return this == &_other ||
           (m_bufferBytes == _other.m_bufferBytes && m_capacityInBytes == _other.m_capacityInBytes);
}

template <FreeListAllocatorPolicy Policy, CoalescingPolicy CoalescePolicy>
bool FreeListAllocator<Policy, CoalescePolicy>::operator!=(
    const FreeListAllocator& _other) const noexcept
{
    return !(*this == _other);
}

} // namespace pieces
