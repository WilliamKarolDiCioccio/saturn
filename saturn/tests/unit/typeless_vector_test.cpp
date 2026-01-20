#include <gtest/gtest.h>

#include <vector>

#include <saturn/ecs/typeless_vector.hpp>

using namespace saturn::ecs;

struct Foo
{
    int x;
    float y;
};

struct Bar
{
    char c;
    double d;
};

TEST(TypelessVectorTest, ConstructWithCapacity)
{
    TypelessVector vec(sizeof(Foo), 4);
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_EQ(vec.stride(), sizeof(Foo));
}

TEST(TypelessVectorTest, PushAndAccess)
{
    TypelessVector vec(sizeof(Foo), 2);

    Foo f1{42, 3.14f};
    Foo f2{7, -1.5f};

    vec.pushBack(&f1);
    vec.pushBack(&f2);

    auto* r1 = reinterpret_cast<Foo*>(vec[0]);
    auto* r2 = reinterpret_cast<Foo*>(vec[1]);

    EXPECT_EQ(r1->x, 42);
    EXPECT_FLOAT_EQ(r1->y, 3.14f);
    EXPECT_EQ(r2->x, 7);
    EXPECT_FLOAT_EQ(r2->y, -1.5f);
}

TEST(TypelessVectorTest, GrowsCapacityAutomatically)
{
    TypelessVector vec(sizeof(int), 1);

    int a = 1, b = 2, c = 3;
    vec.pushBack(&a);
    size_t capBefore = vec.capacity();

    vec.pushBack(&b);
    vec.pushBack(&c);

    EXPECT_GT(vec.capacity(), capBefore);
    EXPECT_EQ(vec.size(), 3);

    EXPECT_EQ(*reinterpret_cast<int*>(vec[0]), 1);
    EXPECT_EQ(*reinterpret_cast<int*>(vec[1]), 2);
    EXPECT_EQ(*reinterpret_cast<int*>(vec[2]), 3);
}

TEST(TypelessVectorTest, ReserveIncreasesCapacity)
{
    TypelessVector vec(sizeof(int), 2);
    EXPECT_EQ(vec.capacity(), 2);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);
}

TEST(TypelessVectorTest, ShrinkToFitReducesCapacity)
{
    TypelessVector vec(sizeof(int), 5);

    int a = 42;
    vec.pushBack(&a);
    vec.pushBack(&a);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec.capacity(), 5);

    vec.shrinkToFit();
    EXPECT_EQ(vec.capacity(), 2);
}

TEST(TypelessVectorTest, ResizeExpandsSize)
{
    TypelessVector vec(sizeof(int), 2);
    vec.resize(5);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
}

TEST(TypelessVectorTest, ClearResetsSizeButNotCapacity)
{
    TypelessVector vec(sizeof(int), 4);

    int a = 42;
    vec.pushBack(&a);
    vec.pushBack(&a);

    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 4); // unchanged
}

TEST(TypelessVectorTest, PopBackRemovesLast)
{
    TypelessVector vec(sizeof(int), 3);

    int a = 5, b = 10;
    vec.pushBack(&a);
    vec.pushBack(&b);
    EXPECT_EQ(vec.size(), 2);

    vec.popBack();
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(*reinterpret_cast<int*>(vec[0]), 5);
}

TEST(TypelessVectorTest, EraseRemovesAndSwapsLast)
{
    TypelessVector vec(sizeof(int), 3);

    int a = 1, b = 2, c = 3;
    vec.pushBack(&a);
    vec.pushBack(&b);
    vec.pushBack(&c);

    EXPECT_EQ(vec.size(), 3);

    vec.erase(1); // remove "2"

    EXPECT_EQ(vec.size(), 2);
    int* r0 = reinterpret_cast<int*>(vec[0]);
    int* r1 = reinterpret_cast<int*>(vec[1]);

    // r1 should now contain c (3)
    EXPECT_EQ(*r0, 1);
    EXPECT_EQ(*r1, 3);
}

TEST(TypelessVectorTest, WorksWithDifferentTypesSameStride)
{
    size_t stride = std::max(sizeof(Foo), sizeof(Bar));
    TypelessVector vec(stride, 2);

    Foo f{1, 2.0f};
    Bar b{'z', 99.9};

    vec.pushBack(&f);
    vec.pushBack(&b);

    Foo* r1 = reinterpret_cast<Foo*>(vec[0]);
    Bar* r2 = reinterpret_cast<Bar*>(vec[1]);

    EXPECT_EQ(r1->x, 1);
    EXPECT_FLOAT_EQ(r1->y, 2.0f);

    EXPECT_EQ(r2->c, 'z');
    EXPECT_DOUBLE_EQ(r2->d, 99.9);
}

TEST(TypelessVectorTest, IterateByStride)
{
    TypelessVector vec(sizeof(int), 3);

    int a = 10, b = 20, c = 30;
    vec.pushBack(&a);
    vec.pushBack(&b);
    vec.pushBack(&c);

    std::vector<int> values;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        values.push_back(*reinterpret_cast<int*>(*it));
    }

    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 10);
    EXPECT_EQ(values[1], 20);
    EXPECT_EQ(values[2], 30);
}
