#include <gtest/gtest.h>

#include <memory>

#include <mosaic/ecs/entity_registry.hpp>

using namespace mosaic::ecs;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Components
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Position
{
    float x, y, z;
};

struct Velocity
{
    float dx, dy, dz;
};

struct Health
{
    int hp, maxHp;
};

struct Tag
{
    int value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Fixture
////////////////////////////////////////////////////////////////////////////////////////////////////

class ECSTest : public ::testing::Test
{
   protected:
    std::unique_ptr<ComponentRegistry> m_compRegistry;
    std::unique_ptr<EntityRegistry> m_entityRegistry;

    void SetUp() override
    {
        m_compRegistry = std::make_unique<ComponentRegistry>(64);

        m_compRegistry->registerComponent<Position>("Position");
        m_compRegistry->registerComponent<Velocity>("Velocity");
        m_compRegistry->registerComponent<Health>("Health");
        m_compRegistry->registerComponent<Tag>("Tag");

        m_entityRegistry = std::make_unique<EntityRegistry>(m_compRegistry.get());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Operations Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, CreateAndDestroyEntity)
{
    auto eid = m_entityRegistry->createEntity<Position>().id;
    EXPECT_EQ(m_entityRegistry->entityCount(), 1);

    m_entityRegistry->destroyEntity(eid);
    EXPECT_EQ(m_entityRegistry->entityCount(), 0);
}

TEST_F(ECSTest, ClearRegistryRemovesAllEntities)
{
    for (int i = 0; i < 10; i++) m_entityRegistry->createEntity<Position>();
    EXPECT_EQ(m_entityRegistry->entityCount(), 10);

    m_entityRegistry->clear();
    EXPECT_EQ(m_entityRegistry->entityCount(), 0);
}

TEST_F(ECSTest, AddAndGetComponent)
{
    auto eid = m_entityRegistry->createEntity<Position>().id;
    m_entityRegistry->addComponents<Velocity>(eid);

    auto result = m_entityRegistry->getComponents<Velocity>(eid);
    ASSERT_TRUE(result.has_value());

    auto& vel = std::get<0>(result.value());
    vel.dx = 42.f;
    EXPECT_EQ(vel.dx, 42.f);

    result = m_entityRegistry->getComponents<Velocity>(eid);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(std::get<0>(result.value()).dx, 42.f);
}

TEST_F(ECSTest, RemoveComponent)
{
    auto eid = m_entityRegistry->createEntity<Position, Velocity>().id;
    m_entityRegistry->removeComponents<Velocity>(eid);

    auto result = m_entityRegistry->getComponents<Velocity>(eid);
    ASSERT_FALSE(result.has_value());
}

TEST_F(ECSTest, DifferentArchetypesForDifferentSignatures)
{
    auto eid1 = m_entityRegistry->createEntity<Position>();
    auto eid2 = m_entityRegistry->createEntity<Position, Velocity>();

    EXPECT_EQ(m_entityRegistry->archetypeCount(), 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// View Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, ViewSingleComponentSetIteratesCorrectly)
{
    for (int i = 0; i < 5; i++) m_entityRegistry->createEntity<Position>();

    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    int count = 0;
    view.forEach(
        [&](EntityMeta, Position& pos)
        {
            pos.x = 1.0f;
            count++;
        });

    EXPECT_EQ(count, 5);
}

TEST_F(ECSTest, ViewMultipleComponentsSetIteratesCorrectly)
{
    for (int i = 0; i < 3; i++) m_entityRegistry->createEntity<Position, Velocity>();

    auto result = m_entityRegistry->viewSet<Position, Velocity>();
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    int count = 0;
    view.forEach(
        [&](EntityMeta, Position& pos, Velocity& vel)
        {
            vel.dx = 2.0f;
            pos.x = vel.dx;
            count++;
        });

    EXPECT_EQ(count, 3);
}

TEST_F(ECSTest, ViewSingleComponentSubsetIteratesCorrectly)
{
    m_entityRegistry->createEntity<Position>();
    m_entityRegistry->createEntity<Position, Velocity>();
    m_entityRegistry->createEntity<Position, Health>();
    m_entityRegistry->createEntity<Velocity>();

    auto result = m_entityRegistry->viewSubset<Position>();
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    int count = 0;
    view.forEach(
        [&](EntityMeta, Position& pos)
        {
            pos.x = 10.0f;
            count++;
        });

    EXPECT_EQ(count, 3); // Should find 3 entities with Position component
}

TEST_F(ECSTest, ViewMultipleComponentsSubsetIteratesCorrectly)
{
    m_entityRegistry->createEntity<Position>();
    m_entityRegistry->createEntity<Position, Velocity>();
    m_entityRegistry->createEntity<Position, Velocity, Health>();
    m_entityRegistry->createEntity<Velocity, Health>();

    auto result = m_entityRegistry->viewSubset<Position, Velocity>();
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    int count = 0;
    view.forEach(
        [&](EntityMeta, Position& pos, Velocity& vel)
        {
            pos.x = vel.dx + 1.0f;
            count++;
        });

    EXPECT_EQ(count, 2); // Should find 2 entities with both Position and Velocity
}

TEST_F(ECSTest, ViewSubsetReturnsNulloptWhenNoMatchingEntities)
{
    m_entityRegistry->createEntity<Position>();
    m_entityRegistry->createEntity<Velocity>();

    auto result = m_entityRegistry->viewSubset<Health>();
    ASSERT_FALSE(result.has_value());
}

TEST_F(ECSTest, ViewSubsetWithUnregisteredComponentThrows)
{
    struct UnregisteredComponent
    {
        int data;
    };

    EXPECT_THROW(m_entityRegistry->viewSubset<UnregisteredComponent>(), std::runtime_error);
}

TEST_F(ECSTest, RemoveNonExistentComponentDoesNotCrash)
{
    auto eid = m_entityRegistry->createEntity<Position>().id;
    EXPECT_NO_THROW(m_entityRegistry->removeComponents<Velocity>(eid));
}

TEST_F(ECSTest, DestroyInvalidEntityDoesNotCrash)
{
    EXPECT_NO_THROW(m_entityRegistry->destroyEntity(EntityID{99999}));
}

TEST_F(ECSTest, EmptyViewIsSafe)
{
    auto result = m_entityRegistry->viewSet<Tag>();
    ASSERT_FALSE(result.has_value());
}
