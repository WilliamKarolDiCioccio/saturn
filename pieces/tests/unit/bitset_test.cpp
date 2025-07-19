#include <gtest/gtest.h>

#include <pieces/containers/bitset.hpp>

using namespace pieces;

TEST(BitSetTest, ConstructorAndSize)
{
    BitSet bs(100);
    EXPECT_EQ(bs.size(), 100);
    EXPECT_TRUE(bs.none());
    EXPECT_FALSE(bs.any());
}

TEST(BitSetTest, SetAndTestBit)
{
    BitSet bs(10);
    bs.setBit(3);
    EXPECT_TRUE(bs.testBit(3));
    EXPECT_FALSE(bs.testBit(2));
    bs.setBit(9);
    EXPECT_TRUE(bs.testBit(9));
}

TEST(BitSetTest, ClearBit)
{
    BitSet bs(8);
    bs.setBit(5);
    EXPECT_TRUE(bs.testBit(5));
    bs.clearBit(5);
    EXPECT_FALSE(bs.testBit(5));
}

TEST(BitSetTest, FlipBit)
{
    BitSet bs(8);
    bs.flipBit(2);
    EXPECT_TRUE(bs.testBit(2));
    bs.flipBit(2);
    EXPECT_FALSE(bs.testBit(2));
}

TEST(BitSetTest, SetAllAndClearAll)
{
    BitSet bs(16);
    bs.setAll();
    for (size_t i = 0; i < bs.size(); ++i) EXPECT_TRUE(bs.testBit(i));
    bs.clearAll();
    for (size_t i = 0; i < bs.size(); ++i) EXPECT_FALSE(bs.testBit(i));
}

TEST(BitSetTest, PopcountAndCount)
{
    BitSet bs(10);
    bs.setBit(0);
    bs.setBit(5);
    bs.setBit(9);
    EXPECT_EQ(bs.popcount(), 3);
    EXPECT_EQ(bs.count(), 3);
}

TEST(BitSetTest, FindFirstSet)
{
    BitSet bs(10);
    EXPECT_EQ(bs.findFirstSet(), 10);
    bs.setBit(7);
    EXPECT_EQ(bs.findFirstSet(), 7);
    bs.setBit(2);
    EXPECT_EQ(bs.findFirstSet(), 2);
}

TEST(BitSetTest, FindFirstSetFrom)
{
    BitSet bs(10);
    bs.setBit(3);
    bs.setBit(7);
    EXPECT_EQ(bs.findFirstSetFrom(0), 3);
    EXPECT_EQ(bs.findFirstSetFrom(4), 7);
    EXPECT_EQ(bs.findFirstSetFrom(8), 10);
}

TEST(BitSetTest, FindFirstClear)
{
    BitSet bs(5);
    bs.setAll();
    EXPECT_EQ(bs.findFirstClear(), 5);
    bs.clearBit(2);
    EXPECT_EQ(bs.findFirstClear(), 2);
}

TEST(BitSetTest, BitwiseOperators)
{
    BitSet a(8), b(8);
    a.setBit(1);
    a.setBit(3);
    b.setBit(3);
    b.setBit(4);

    BitSet c = a & b;
    EXPECT_TRUE(c.testBit(3));
    EXPECT_FALSE(c.testBit(1));
    EXPECT_FALSE(c.testBit(4));

    c = a | b;
    EXPECT_TRUE(c.testBit(1));
    EXPECT_TRUE(c.testBit(3));
    EXPECT_TRUE(c.testBit(4));

    c = a ^ b;
    EXPECT_TRUE(c.testBit(1));
    EXPECT_FALSE(c.testBit(3));
    EXPECT_TRUE(c.testBit(4));
}

TEST(BitSetTest, BitwiseAssignmentOperators)
{
    BitSet a(8), b(8);
    a.setBit(2);
    b.setBit(2);
    b.setBit(5);

    a &= b;
    EXPECT_TRUE(a.testBit(2));
    EXPECT_FALSE(a.testBit(5));

    a |= b;
    EXPECT_TRUE(a.testBit(5));

    a ^= b;
    EXPECT_FALSE(a.testBit(2));
    EXPECT_FALSE(a.testBit(5));
}

TEST(BitSetTest, CopyAndMoveSemantics)
{
    BitSet a(8);
    a.setBit(4);
    BitSet b = a;
    EXPECT_TRUE(b.testBit(4));
    b.setBit(5);
    EXPECT_FALSE(a.testBit(5));

    BitSet c(8);
    c = a;
    EXPECT_TRUE(c.testBit(4));

    BitSet d(std::move(a));
    EXPECT_TRUE(d.testBit(4));
    EXPECT_EQ(a.size(), 0);

    BitSet e(8);
    e = std::move(b);
    EXPECT_TRUE(e.testBit(4));
    EXPECT_TRUE(e.testBit(5));
    EXPECT_EQ(b.size(), 0);
}

TEST(BitSetTest, ExceptionOnSizeMismatch)
{
    BitSet a(8), b(9);
    EXPECT_THROW(a & b, std::invalid_argument);
    EXPECT_THROW(a | b, std::invalid_argument);
    EXPECT_THROW(a ^ b, std::invalid_argument);
    EXPECT_THROW(a &= b, std::invalid_argument);
    EXPECT_THROW(a |= b, std::invalid_argument);
    EXPECT_THROW(a ^= b, std::invalid_argument);
}

TEST(BitSetTest, ExceptionOnZeroSize) { EXPECT_THROW(BitSet(0), std::invalid_argument); }
