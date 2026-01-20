#include <gtest/gtest.h>

#include <random>
#include <numeric>
#include <algorithm>
#include <array>

#include <saturn/ecs/typeless_sparse_set.hpp>

using namespace saturn::ecs;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Components
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Position
{
    float x, y, z;

    Position() = default;
    Position(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    bool operator==(const Position& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct Velocity
{
    double dx, dy;

    Velocity() = default;
    Velocity(double dx_, double dy_) : dx(dx_), dy(dy_) {}

    bool operator==(const Velocity& other) const { return dx == other.dx && dy == other.dy; }
};

struct Health
{
    int current, maximum;

    Health() = default;
    Health(int current_, int maximum_) : current(current_), maximum(maximum_) {}

    bool operator==(const Health& other) const
    {
        return current == other.current && maximum == other.maximum;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture
////////////////////////////////////////////////////////////////////////////////////////////////////

class TypelessSparseSetMixedTest : public ::testing::Test
{
   protected:
    TypelessSparseSet<64, false> healthSet{sizeof(Health)};
    TypelessSparseSet<64, false> velocitySet{sizeof(Velocity)};

    void SetUp() override
    {
        healthSet.clear();
        velocitySet.clear();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Stress Tests with Mixed Component Types
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(TypelessSparseSetMixedTest, MultiComponentECSSimulation)
{
    // Simulate a simple ECS with Health and Velocity components
    const uint32_t numEntities = 10;

    // Create entities with both Health and Velocity components
    for (uint32_t entity = 0; entity < numEntities; ++entity)
    {
        Health health(100 + entity, 100 + entity);
        Velocity velocity(entity * 0.5, entity * 0.3);

        healthSet.insert(entity, &health);
        velocitySet.insert(entity, &velocity);
    }

    EXPECT_EQ(healthSet.size(), numEntities);
    EXPECT_EQ(velocitySet.size(), numEntities);

    // Simulate a system that processes entities with both components
    uint32_t processedCount = 0;
    for (uint32_t entity = 0; entity < numEntities * 2; ++entity) // Test beyond range
    {
        if (healthSet.contains(entity) && velocitySet.contains(entity))
        {
            Health* health = healthSet.getTyped<Health>(entity);
            Velocity* velocity = velocitySet.getTyped<Velocity>(entity);

            ASSERT_NE(health, nullptr);
            ASSERT_NE(velocity, nullptr);

            // Simulate some processing
            health->current -= 1;
            velocity->dx *= 0.99;
            velocity->dy *= 0.99;

            ++processedCount;
        }
    }

    EXPECT_EQ(processedCount, numEntities);

    // Verify modifications
    for (uint32_t entity = 0; entity < numEntities; ++entity)
    {
        Health* health = healthSet.getTyped<Health>(entity);
        Velocity* velocity = velocitySet.getTyped<Velocity>(entity);

        EXPECT_EQ(health->current, 99 + entity);  // Reduced by 1
        EXPECT_EQ(health->maximum, 100 + entity); // Unchanged

        EXPECT_NEAR(velocity->dx, entity * 0.5 * 0.99, 1e-10);
        EXPECT_NEAR(velocity->dy, entity * 0.3 * 0.99, 1e-10);
    }
}

TEST_F(TypelessSparseSetMixedTest, SparseEntityDistribution)
{
    // Test with sparse entity IDs (not consecutive)
    std::vector<uint32_t> sparseEntities = {1, 5, 17, 42, 100, 256, 512, 1000, 2048, 4096};

    // Insert Health components for sparse entities
    for (size_t i = 0; i < sparseEntities.size(); ++i)
    {
        Health health(static_cast<int>(i * 10), static_cast<int>(i * 10 + 50));
        healthSet.insert(sparseEntities[i], &health);
    }

    EXPECT_EQ(healthSet.size(), sparseEntities.size());

    // Verify all sparse entities are accessible
    for (size_t i = 0; i < sparseEntities.size(); ++i)
    {
        EXPECT_TRUE(healthSet.contains(sparseEntities[i]));

        Health* health = healthSet.getTyped<Health>(sparseEntities[i]);
        ASSERT_NE(health, nullptr);
        EXPECT_EQ(health->current, static_cast<int>(i * 10));
        EXPECT_EQ(health->maximum, static_cast<int>(i * 10 + 50));
    }

    // Test that non-existent entities return nullptr
    std::vector<uint32_t> nonExistentEntities = {0, 2, 3, 43, 101, 4097};
    for (auto entity : nonExistentEntities)
    {
        EXPECT_FALSE(healthSet.contains(entity));
        EXPECT_EQ(healthSet.getTyped<Health>(entity), nullptr);
    }
}
