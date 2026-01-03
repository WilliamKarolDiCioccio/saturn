#include <gtest/gtest.h>

#include <pieces/core/templates.hpp>
#include <pieces/memory/base_allocator.hpp>
#include <pieces/memory/proxy_allocator.hpp>
#include <pieces/memory/contiguous_allocator.hpp>
#include <pieces/memory/pool_allocator.hpp>
#include <pieces/memory/freelist_allocator.hpp>

using namespace pieces;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for LinearAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class LinearAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 1024;
    LinearAllocator<int> linearAlloc{kCapacity};
};

TEST_F(LinearAllocatorTest, AllocateWithinCapacity)
{
    int* ptr1 = linearAlloc.allocate(10);
    int* ptr2 = linearAlloc.allocate(20);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    linearAlloc.construct(ptr1, 42);
    linearAlloc.construct(ptr2, 84);

    EXPECT_EQ(ptr1[0], 42);
    EXPECT_EQ(ptr2[0], 84);
}

TEST_F(LinearAllocatorTest, AllocateZeroReturnsNullptr)
{
    int* ptr = linearAlloc.allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LinearAllocatorTest, AllocateBeyondCapacityReturnsNullptr)
{
    EXPECT_EQ(linearAlloc.allocate(kCapacity + 1), nullptr);
}

TEST_F(LinearAllocatorTest, ConstructAndDestroyObject)
{
    struct Dummy
    {
        int x;
        Dummy(int val) : x(val) {}
    };

    LinearAllocator<Dummy> dummyAlloc(128);
    Dummy* ptr = dummyAlloc.allocate(1);

    dummyAlloc.construct(ptr, 99);
    EXPECT_EQ(ptr->x, 99);

    dummyAlloc.destroy(ptr);
}

TEST_F(LinearAllocatorTest, ResetAllowsReuse)
{
    int* ptr1 = linearAlloc.allocate(10);
    linearAlloc.reset();
    int* ptr2 = linearAlloc.allocate(10);

    EXPECT_EQ(ptr1, ptr2);
}

TEST_F(LinearAllocatorTest, MoveConstructorTransfersOwnership)
{
    LinearAllocator<int> alloc1(128);
    int* ptr = alloc1.allocate(2);
    alloc1.construct(ptr, 7);

    LinearAllocator<int> alloc2(std::move(alloc1));

    EXPECT_EQ(ptr[0], 7);
    EXPECT_EQ(alloc1.allocate(1), nullptr);
}

TEST_F(LinearAllocatorTest, MoveAssignmentTransfersOwnership)
{
    LinearAllocator<int> alloc1(128);
    int* ptr = alloc1.allocate(2);
    alloc1.construct(ptr, 8);

    LinearAllocator<int> alloc2(64);
    alloc2 = std::move(alloc1);

    EXPECT_EQ(ptr[0], 8);
    EXPECT_EQ(alloc1.allocate(1), nullptr);
}

TEST_F(LinearAllocatorTest, EqualityOperators)
{
    LinearAllocator<int> alloc1(128);
    LinearAllocator<int> alloc2(128);

    EXPECT_FALSE(alloc1 == alloc2);
    EXPECT_TRUE(alloc1 != alloc2);

    LinearAllocator<int> alloc3(std::move(alloc1));
    EXPECT_TRUE(alloc3 != alloc2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for StackAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class StackAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 256;
    StackAllocator<int> stackAlloc{kCapacity};
};

TEST_F(StackAllocatorTest, LifoDeallocation)
{
    int* ptr1 = stackAlloc.allocate(1);
    int* ptr2 = stackAlloc.allocate(1);

    stackAlloc.deallocate(ptr2, 1);
    EXPECT_EQ(stackAlloc.used(), 1);

    stackAlloc.deallocate(ptr1, 1);
    EXPECT_EQ(stackAlloc.used(), 0);
}

TEST_F(StackAllocatorTest, DeallocateWrongOrderThrows)
{
    int* ptr1 = stackAlloc.allocate(1);
    int* ptr2 = stackAlloc.allocate(1);
    EXPECT_THROW(stackAlloc.deallocate(ptr1, 1), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for CircularAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class CircularAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 64;
    CircularAllocator<int> circAlloc{kCapacity};
};

TEST_F(CircularAllocatorTest, WrapsOnOverflow)
{
    int* ptr1 = circAlloc.allocate(15);

    size_t largeCount = CircularAllocatorTest::kCapacity - circAlloc.used();
    int* wrapped = circAlloc.allocate(largeCount);
    EXPECT_NE(wrapped, nullptr);

    int* ptr2 = circAlloc.allocate(1);
    EXPECT_EQ(ptr2, ptr1);
}

TEST_F(CircularAllocatorTest, AllocateZeroReturnsNullptr)
{
    EXPECT_EQ(circAlloc.allocate(0), nullptr);
}

TEST_F(CircularAllocatorTest, AllocateTooLargeAlwaysNullptr)
{
    EXPECT_EQ(circAlloc.allocate(CircularAllocatorTest::kCapacity + 1), nullptr);
}

class AutomaticIdxPoolAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 128;
    AutomaticIndexingPoolAllocator<int> poolAlloc{kCapacity};
};

TEST_F(AutomaticIdxPoolAllocatorTest, DeallocateRandomOrderWorks)
{
    int* ptr1 = poolAlloc.allocate(5);
    int* ptr2 = poolAlloc.allocate(3);

    poolAlloc.deallocate(ptr2, 3);
    poolAlloc.deallocate(ptr1, 5);

    EXPECT_EQ(poolAlloc.used(), 0);
}

TEST_F(AutomaticIdxPoolAllocatorTest, AllocateCountFailsIfNotEnoughContiguousSlots)
{
    int* ptr1 = poolAlloc.allocate(kCapacity - 2);

    EXPECT_EQ(poolAlloc.allocate(4), nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for ManualIndexingPoolAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class ManualIdxPoolAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 128;
    ManualIndexingPoolAllocator<int> poolAlloc{kCapacity};
};

TEST_F(ManualIdxPoolAllocatorTest, AllocateAtIndexWorks)
{
    int* ptr1 = poolAlloc.allocateAt(0, 10);
    int* ptr2 = poolAlloc.allocateAt(9, 1);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(ptr2, nullptr);

    poolAlloc.deallocateAt(0, 10);

    EXPECT_EQ(poolAlloc.used(), 0);
}

TEST_F(ManualIdxPoolAllocatorTest, AllocateAtIndexFailsIfIndexOutOfBounds)
{
    EXPECT_THROW(poolAlloc.allocateAt(130, 10), std::runtime_error);
}

TEST_F(ManualIdxPoolAllocatorTest, AllocateAtIndexFailsIfNotEnoughtContiguousSlots)
{
    int* ptr1 = poolAlloc.allocateAt(5, 10);
    int* ptr2 = poolAlloc.allocateAt(0, 10);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(ptr2, nullptr);

    poolAlloc.deallocateAt(5, 10);
}

TEST_F(ManualIdxPoolAllocatorTest, DeallocateAndAllocateAtIndexCorrectlySetBitmasks)
{
    int* ptr1 = poolAlloc.allocateAt(5, 10);
    int* ptr2 = poolAlloc.allocateAt(20, 40);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);

    EXPECT_TRUE(poolAlloc.owns(ptr1));
    EXPECT_TRUE(poolAlloc.owns(ptr2));

    poolAlloc.deallocateAt(5, 10);
    poolAlloc.deallocateAt(20, 40);

    EXPECT_FALSE(poolAlloc.owns(ptr1));
    EXPECT_FALSE(poolAlloc.owns(ptr2));

    EXPECT_EQ(poolAlloc.used(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for FreeListAllocator (First-Fit with Deferred Coalescing)
////////////////////////////////////////////////////////////////////////////////////////////////////

class FreeListAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 4096;
    FirstFitAllocator<CoalescingPolicy::deferred> freeListAlloc{kCapacity};
};

TEST_F(FreeListAllocatorTest, AllocateWithinCapacity)
{
    void* ptr1 = freeListAlloc.allocate(64);
    void* ptr2 = freeListAlloc.allocate(128);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    EXPECT_TRUE(freeListAlloc.owns(ptr1));
    EXPECT_TRUE(freeListAlloc.owns(ptr2));

    // Write to allocated memory to verify it works
    int* intPtr1 = static_cast<int*>(ptr1);
    intPtr1[0] = 42;
    EXPECT_EQ(intPtr1[0], 42);

    freeListAlloc.deallocate(ptr1, 64);
    freeListAlloc.deallocate(ptr2, 128);
}

TEST_F(FreeListAllocatorTest, AllocateZeroReturnsNullptr)
{
    void* ptr = freeListAlloc.allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FreeListAllocatorTest, AllocateBeyondCapacityReturnsNullptr)
{
    void* ptr = freeListAlloc.allocate(kCapacity + 1);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FreeListAllocatorTest, DeallocateNullptrIsSafe)
{
    freeListAlloc.deallocate(nullptr, 64);
    EXPECT_EQ(freeListAlloc.used(), 0);
}

TEST_F(FreeListAllocatorTest, RandomOrderDeallocation)
{
    void* ptr1 = freeListAlloc.allocate(64);
    void* ptr2 = freeListAlloc.allocate(128);
    void* ptr3 = freeListAlloc.allocate(256);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // Deallocate in random order (middle first)
    freeListAlloc.deallocate(ptr2, 128);
    freeListAlloc.deallocate(ptr3, 256);
    freeListAlloc.deallocate(ptr1, 64);

    EXPECT_EQ(freeListAlloc.used(), 0);
}

TEST_F(FreeListAllocatorTest, BlockSplitting)
{
    // Allocate small block from large buffer, should split
    void* ptr1 = freeListAlloc.allocate(64);

    // Should have split the initial block
    EXPECT_GT(freeListAlloc.getFreeBlockCount(), 0);
    EXPECT_GT(freeListAlloc.getLargestFreeBlock(), 0);

    freeListAlloc.deallocate(ptr1, 64);
}

TEST_F(FreeListAllocatorTest, DeferredCoalescingOnAllocationFailure)
{
    // Create fragmented state
    void* ptr1 = freeListAlloc.allocate(512);
    void* ptr2 = freeListAlloc.allocate(512);
    void* ptr3 = freeListAlloc.allocate(512);
    void* ptr4 = freeListAlloc.allocate(512);

    // Deallocate every other block to create fragmentation
    freeListAlloc.deallocate(ptr1, 512);
    freeListAlloc.deallocate(ptr3, 512);

    // At this point we have fragmented free blocks
    EXPECT_GT(freeListAlloc.getFreeBlockCount(), 1);

    // Try to allocate large block that doesn't fit in any single free block
    // This should trigger deferred coalescing if fragments are adjacent
    void* largePtr = freeListAlloc.allocate(512);

    // Clean up
    freeListAlloc.deallocate(ptr2, 512);
    freeListAlloc.deallocate(ptr4, 512);
    if (largePtr) freeListAlloc.deallocate(largePtr, 512);
}

TEST_F(FreeListAllocatorTest, ResetClearsAllocator)
{
    void* ptr1 = freeListAlloc.allocate(256);
    void* ptr2 = freeListAlloc.allocate(512);

    EXPECT_GT(freeListAlloc.used(), 0);

    freeListAlloc.reset();

    EXPECT_EQ(freeListAlloc.used(), 0);
    EXPECT_EQ(freeListAlloc.getFreeBlockCount(), 1);

    // Should be able to allocate again
    void* ptr3 = freeListAlloc.allocate(1024);
    EXPECT_NE(ptr3, nullptr);

    freeListAlloc.deallocate(ptr3, 1024);
}

TEST_F(FreeListAllocatorTest, MoveConstructorTransfersOwnership)
{
    void* ptr = freeListAlloc.allocate(256);
    ASSERT_NE(ptr, nullptr);

    int* intPtr = static_cast<int*>(ptr);
    intPtr[0] = 99;

    FirstFitAllocator<CoalescingPolicy::deferred> movedAlloc(std::move(freeListAlloc));

    EXPECT_EQ(intPtr[0], 99);
    EXPECT_TRUE(movedAlloc.owns(ptr));

    // Original should be invalidated
    EXPECT_EQ(freeListAlloc.allocate(64), nullptr);

    movedAlloc.deallocate(ptr, 256);
}

TEST_F(FreeListAllocatorTest, MoveAssignmentTransfersOwnership)
{
    void* ptr = freeListAlloc.allocate(256);
    ASSERT_NE(ptr, nullptr);

    FirstFitAllocator<CoalescingPolicy::deferred> otherAlloc{2048};
    otherAlloc = std::move(freeListAlloc);

    EXPECT_TRUE(otherAlloc.owns(ptr));

    // Original should be invalidated
    EXPECT_EQ(freeListAlloc.capacity(), 0);

    otherAlloc.deallocate(ptr, 256);
}

TEST_F(FreeListAllocatorTest, FragmentationMetrics)
{
    // Allocate and create fragmentation
    void* ptr1 = freeListAlloc.allocate(512);
    void* ptr2 = freeListAlloc.allocate(512);
    void* ptr3 = freeListAlloc.allocate(512);

    freeListAlloc.deallocate(ptr1, 512);
    freeListAlloc.deallocate(ptr3, 512);

    // Should have fragmented free blocks
    size_t freeCount = freeListAlloc.getFreeBlockCount();
    float fragRatio = freeListAlloc.getFragmentationRatio();

    EXPECT_GT(freeCount, 0);
    EXPECT_GE(fragRatio, 0.0f);
    EXPECT_LE(fragRatio, 1.0f);

    freeListAlloc.deallocate(ptr2, 512);
}

TEST_F(FreeListAllocatorTest, DoubleFreeIsIgnored)
{
    void* ptr = freeListAlloc.allocate(256);
    ASSERT_NE(ptr, nullptr);

    freeListAlloc.deallocate(ptr, 256);
    size_t usedAfterFirst = freeListAlloc.used();

    // Double-free should be ignored
    freeListAlloc.deallocate(ptr, 256);
    EXPECT_EQ(freeListAlloc.used(), usedAfterFirst);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for FreeListAllocator with Immediate Coalescing
////////////////////////////////////////////////////////////////////////////////////////////////////

class FreeListAllocatorImmediateCoalescingTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 4096;
    FirstFitAllocator<CoalescingPolicy::immediate> freeListAlloc{kCapacity};
};

TEST_F(FreeListAllocatorImmediateCoalescingTest, ImmediateCoalescingMergesBlocks)
{
    void* ptr1 = freeListAlloc.allocate(512);
    void* ptr2 = freeListAlloc.allocate(512);

    // Deallocate in reverse order, should coalesce immediately
    freeListAlloc.deallocate(ptr2, 512);
    freeListAlloc.deallocate(ptr1, 512);

    // After immediate coalescing, should have fewer free blocks
    EXPECT_EQ(freeListAlloc.getFreeBlockCount(), 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for BestFitAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class BestFitAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 4096;
    BestFitAllocator<CoalescingPolicy::deferred> bestFitAlloc{kCapacity};
};

TEST_F(BestFitAllocatorTest, AllocatesSmallestSuitableBlock)
{
    // Create fragmented blocks of different sizes
    void* ptr1 = bestFitAlloc.allocate(256);
    void* ptr2 = bestFitAlloc.allocate(512);
    void* ptr3 = bestFitAlloc.allocate(1024);

    bestFitAlloc.deallocate(ptr1, 256);
    bestFitAlloc.deallocate(ptr2, 512);
    bestFitAlloc.deallocate(ptr3, 1024);

    // Reset and create specific fragmentation pattern
    bestFitAlloc.reset();

    ptr1 = bestFitAlloc.allocate(128);
    ptr2 = bestFitAlloc.allocate(512);
    ptr3 = bestFitAlloc.allocate(256);

    bestFitAlloc.deallocate(ptr1, 128);
    bestFitAlloc.deallocate(ptr3, 256);

    // Allocate 200 bytes - best fit should choose 256-byte block over larger blocks
    void* ptr4 = bestFitAlloc.allocate(200);
    EXPECT_NE(ptr4, nullptr);

    bestFitAlloc.deallocate(ptr2, 512);
    bestFitAlloc.deallocate(ptr4, 200);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture for WorstFitAllocator
////////////////////////////////////////////////////////////////////////////////////////////////////

class WorstFitAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 4096;
    WorstFitAllocator<CoalescingPolicy::deferred> worstFitAlloc{kCapacity};
};

TEST_F(WorstFitAllocatorTest, AllocatesLargestBlock)
{
    void* ptr1 = worstFitAlloc.allocate(256);
    void* ptr2 = worstFitAlloc.allocate(512);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    // Worst-fit should allocate from largest available block
    size_t largestBefore = worstFitAlloc.getLargestFreeBlock();
    void* ptr3 = worstFitAlloc.allocate(128);

    EXPECT_NE(ptr3, nullptr);

    worstFitAlloc.deallocate(ptr1, 256);
    worstFitAlloc.deallocate(ptr2, 512);
    worstFitAlloc.deallocate(ptr3, 128);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// General FreeListAllocator Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FreeListAllocatorGeneralTest, ConstructorThrowsOnTooSmallCapacity)
{
    EXPECT_THROW(FirstFitAllocator<>(10), std::invalid_argument);
}

TEST(FreeListAllocatorGeneralTest, OwnsReturnsFalseForInvalidPointer)
{
    FirstFitAllocator<> alloc{1024};
    int x = 42;
    EXPECT_FALSE(alloc.owns(&x));
}

TEST(FreeListAllocatorGeneralTest, CapacityUsedAvailable)
{
    FirstFitAllocator<> alloc{2048};

    size_t cap = alloc.capacity();
    EXPECT_GT(cap, 0);
    EXPECT_EQ(alloc.used(), 0);
    EXPECT_EQ(alloc.available(), cap);

    void* ptr = alloc.allocate(256);
    ASSERT_NE(ptr, nullptr);

    EXPECT_GT(alloc.used(), 0);
    EXPECT_LT(alloc.available(), cap);
    EXPECT_EQ(alloc.used() + alloc.available(), cap);

    alloc.deallocate(ptr, 256);
}

TEST(FreeListAllocatorGeneralTest, EqualityOperators)
{
    FirstFitAllocator<> alloc1{1024};
    FirstFitAllocator<> alloc2{1024};

    EXPECT_NE(alloc1, alloc2);
    EXPECT_EQ(alloc1, alloc1);

    FirstFitAllocator<> alloc3{std::move(alloc1)};
    EXPECT_EQ(alloc3, alloc3);
}
