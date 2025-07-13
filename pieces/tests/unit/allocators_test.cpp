#pragma once

#include <gtest/gtest.h>

#include "pieces/allocators.hpp"

namespace pieces
{

class LinearAllocatorTest : public ::testing::Test
{
   protected:
    static constexpr size_t kCapacity = 1024;
    LinearAllocator<int> allocator{kCapacity};
};

TEST_F(LinearAllocatorTest, AllocateWithinCapacity)
{
    int* ptr1 = allocator.allocate(10);
    int* ptr2 = allocator.allocate(20);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    allocator.construct(ptr1, 42);
    allocator.construct(ptr2, 84);

    EXPECT_EQ(ptr1[0], 42);
    EXPECT_EQ(ptr2[0], 84);
}

TEST_F(LinearAllocatorTest, AllocateZeroReturnsNullptr)
{
    int* ptr = allocator.allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LinearAllocatorTest, AllocateBeyondCapacityThrows)
{
    EXPECT_EQ(allocator.allocate(kCapacity + 1), nullptr);
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
    int* ptr1 = allocator.allocate(10);
    allocator.reset();
    int* ptr2 = allocator.allocate(10);

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

} // namespace pieces
