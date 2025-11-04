#include <gtest/gtest.h>

#include <pieces/core/templates.hpp>
#include <pieces/memory/base_allocator.hpp>
#include <pieces/memory/proxy_allocator.hpp>
#include <pieces/memory/contiguous_allocator.hpp>
#include <pieces/memory/pool_allocator.hpp>

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
