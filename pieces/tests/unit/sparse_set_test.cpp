#include <gtest/gtest.h>

#include <random>
#include <numeric>
#include <algorithm>

#include <pieces/containers/sparse_set.hpp>

using namespace pieces;

using K = size_t;
using T = int;
using SparseSetType = SparseSet<K, T, 64, true>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture
////////////////////////////////////////////////////////////////////////////////////////////////////

class SparseSetTest : public ::testing::Test
{
   protected:
    SparseSetType set;
};

TEST_F(SparseSetTest, InsertAndContains)
{
    EXPECT_FALSE(set.contains(1));

    set.insert(1, 100);

    EXPECT_TRUE(set.contains(1));
    EXPECT_EQ(set.get(1).unwrap(), 100);

    set.insert(1, 200);

    EXPECT_TRUE(set.contains(1));
    EXPECT_EQ(set.get(1).unwrap(), 200);
}

TEST_F(SparseSetTest, RemoveKey)
{
    set.insert(3, 300);
    set.insert(5, 500);

    EXPECT_TRUE(set.contains(3));
    EXPECT_TRUE(set.contains(5));

    set.remove(3);

    EXPECT_FALSE(set.contains(3));
    EXPECT_TRUE(set.contains(5));
}

TEST_F(SparseSetTest, GetMissingReturn) { EXPECT_EQ(set.get(10).error(), ErrorCode::out_of_range); }

TEST_F(SparseSetTest, ClearEmptiesSet)
{
    set.insert(7, 700);
    set.insert(8, 800);

    EXPECT_EQ(set.size(), 2);

    set.clear();

    EXPECT_EQ(set.size(), 0);
    EXPECT_FALSE(set.contains(7));
    EXPECT_FALSE(set.contains(8));
}

TEST_F(SparseSetTest, ReserveCapacity)
{
    // Reserve sparse capacity for keys up to 100 and values count 5
    set.reserve(100, 5);

    for (K key = 0; key < 5; ++key)
    {
        set.insert(key, static_cast<T>(key * 10));
    }

    for (K key = 0; key < 5; ++key)
    {
        EXPECT_TRUE(set.contains(key));
        EXPECT_EQ(set.get(key).unwrap(), static_cast<T>(key * 10));
    }

    EXPECT_EQ(set.size(), 5);
}

TEST_F(SparseSetTest, DenseIterationOrder)
{
    set.insert(10, 1000);
    set.insert(20, 2000);
    set.insert(15, 1500);

    const auto& keys = set.keys();
    const auto& values = set.values();

    ASSERT_EQ(keys.size(), 3);
    ASSERT_EQ(values.size(), 3);

    EXPECT_EQ(keys[0], 10);
    EXPECT_EQ(values[0], 1000);
    EXPECT_EQ(keys[1], 20);
    EXPECT_EQ(values[1], 2000);
    EXPECT_EQ(keys[2], 15);
    EXPECT_EQ(values[2], 1500);
}

TEST_F(SparseSetTest, GetByReference)
{
    set.insert(4, 400);

    auto result = set.get(4);

    EXPECT_EQ(result.unwrap(), 400);

    result.unwrap() = 500;

    EXPECT_EQ(set.get(4).unwrap(), 500);
}

TEST_F(SparseSetTest, AggressiveReclaim)
{
    const size_t keyCount = 1024;

    std::vector<K> keys(keyCount);

    std::iota(keys.begin(), keys.end(), 0);

    std::mt19937 rng(42);

    for (int iter = 0; iter < 100; ++iter)
    {
        std::shuffle(keys.begin(), keys.end(), rng);

        // Insert all
        for (auto k : keys) set.insert(k, static_cast<T>(k));

        EXPECT_EQ(set.size(), keyCount);

        // Remove half randomly
        for (size_t i = 0; i < keyCount / 2; ++i)
        {
            set.remove(keys[i]);
        }

        EXPECT_EQ(set.size(), keyCount / 2);

        // Remove the rest
        for (size_t i = keyCount / 2; i < keyCount; ++i)
        {
            set.remove(keys[i]);
        }

        EXPECT_EQ(set.size(), static_cast<size_t>(0));
    }
}

TEST_F(SparseSetTest, PageBoundaryReinsert)
{
    // Keys at the very edges of page 0 and page 1 (64 is the page size)
    std::array<K, 4> edgeKeys = {64 - 1, 64, 64 + 1, 2 * 64 - 1};

    // Insert them
    for (auto k : edgeKeys) set.insert(k, static_cast<T>(k));
    EXPECT_EQ(set.size(), edgeKeys.size());

    // Remove them
    for (auto k : edgeKeys) set.remove(k);

    EXPECT_EQ(set.size(), 0);

    // Re-insert and check again
    for (auto k : edgeKeys)
    {
        set.insert(k, static_cast<T>(k + 10));
        EXPECT_TRUE(set.contains(k));
        EXPECT_EQ(set.get(k).unwrap(), static_cast<T>(k + 10));
    }

    EXPECT_EQ(set.size(), edgeKeys.size());
}
