#include <gtest/gtest.h>

#include <memory>

#include <saturn/ecs/entity_registry.hpp>

using namespace saturn::ecs;

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
// Single Entity API Tests
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

    auto result = m_entityRegistry->getComponentsForEntity<Velocity>(eid);
    ASSERT_TRUE(result.has_value());

    auto& vel = std::get<0>(result.value());
    vel.dx = 42.f;
    EXPECT_EQ(vel.dx, 42.f);

    result = m_entityRegistry->getComponentsForEntity<Velocity>(eid);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(std::get<0>(result.value()).dx, 42.f);
}

TEST_F(ECSTest, RemoveComponent)
{
    auto eid = m_entityRegistry->createEntity<Position, Velocity>().id;
    m_entityRegistry->removeComponents<Velocity>(eid);

    auto result = m_entityRegistry->getComponentsForEntity<Velocity>(eid);
    ASSERT_FALSE(result.has_value());
}

TEST_F(ECSTest, DifferentArchetypesForDifferentSignatures)
{
    auto eid1 = m_entityRegistry->createEntity<Position>();
    auto eid2 = m_entityRegistry->createEntity<Position, Velocity>();

    EXPECT_EQ(m_entityRegistry->archetypeCount(), 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Bulk Operations API Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, CreateEntityBulkCreatesMultipleEntities)
{
    auto metas = m_entityRegistry->createEntityBulk<Position, Velocity>(10);

    EXPECT_EQ(metas.size(), 10);
    EXPECT_EQ(m_entityRegistry->entityCount(), 10);

    // All entities should be in same archetype
    EXPECT_EQ(m_entityRegistry->archetypeCount(), 1);
}

TEST_F(ECSTest, CreateEntityBulkWithZeroCountReturnsEmpty)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(0);

    EXPECT_TRUE(metas.empty());
    EXPECT_EQ(m_entityRegistry->entityCount(), 0);
}

TEST_F(ECSTest, CreateEntityBulkEntitiesHaveDefaultInitializedComponents)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(5);

    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach(
        [&](EntityMeta, Position& pos)
        {
            // Default initialized floats are indeterminate, but structure should exist
            pos.x = 42.0f;
            count++;
        });

    EXPECT_EQ(count, 5);
}

TEST_F(ECSTest, CreateEntityBulkWithUnregisteredComponentThrows)
{
    struct UnregisteredComponent
    {
        int data;
    };

    EXPECT_THROW(m_entityRegistry->createEntityBulk<UnregisteredComponent>(5), std::runtime_error);
}

TEST_F(ECSTest, DestroyEntityBulkRemovesMultipleEntities)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(10);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    auto notFound = m_entityRegistry->destroyEntityBulk(eids);

    EXPECT_TRUE(notFound.empty());
    EXPECT_EQ(m_entityRegistry->entityCount(), 0);
}

TEST_F(ECSTest, DestroyEntityBulkWithInvalidIDsReturnsNotFoundList)
{
    m_entityRegistry->createEntityBulk<Position>(3);

    std::vector<EntityID> eids = {999, 1000, 1001};
    auto notFound = m_entityRegistry->destroyEntityBulk(eids);

    EXPECT_EQ(notFound.size(), 3);
    EXPECT_EQ(m_entityRegistry->entityCount(), 3);
}

TEST_F(ECSTest, DestroyEntityBulkWithMixedValidAndInvalidIDs)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(5);

    std::vector<EntityID> eids;
    eids.push_back(metas[0].id);
    eids.push_back(999); // invalid
    eids.push_back(metas[2].id);

    auto notFound = m_entityRegistry->destroyEntityBulk(eids);

    EXPECT_EQ(notFound.size(), 1);
    EXPECT_EQ(notFound[0], 999);
    EXPECT_EQ(m_entityRegistry->entityCount(), 3);
}

TEST_F(ECSTest, DestroyEntityBulkWithEmptyVectorIsSafe)
{
    m_entityRegistry->createEntityBulk<Position>(5);

    std::vector<EntityID> empty;
    auto notFound = m_entityRegistry->destroyEntityBulk(empty);

    EXPECT_TRUE(notFound.empty());
    EXPECT_EQ(m_entityRegistry->entityCount(), 5);
}

TEST_F(ECSTest, DestroyEntityBulkHandlesEntitiesAcrossMultipleArchetypes)
{
    auto metas1 = m_entityRegistry->createEntityBulk<Position>(3);
    auto metas2 = m_entityRegistry->createEntityBulk<Position, Velocity>(2);
    auto metas3 = m_entityRegistry->createEntityBulk<Health>(4);

    std::vector<EntityID> eids;
    eids.push_back(metas1[0].id);
    eids.push_back(metas2[1].id);
    eids.push_back(metas3[2].id);

    auto notFound = m_entityRegistry->destroyEntityBulk(eids);

    EXPECT_TRUE(notFound.empty());
    EXPECT_EQ(m_entityRegistry->entityCount(), 6); // 9 - 3
}

TEST_F(ECSTest, AddComponentsBulkModifiesMultipleEntities)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(5);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    auto notFound = m_entityRegistry->addComponentsBulk<Velocity>(eids);

    EXPECT_TRUE(notFound.empty());

    // Entities should now be in archetype with both Position and Velocity
    auto result = m_entityRegistry->viewSet<Position, Velocity>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Velocity&) { count++; });

    EXPECT_EQ(count, 5);
}

TEST_F(ECSTest, AddComponentsBulkWithEmptyVectorReturnsEmpty)
{
    m_entityRegistry->createEntityBulk<Position>(3);

    std::vector<EntityID> empty;
    auto notFound = m_entityRegistry->addComponentsBulk<Velocity>(empty);

    EXPECT_TRUE(notFound.empty());
}

TEST_F(ECSTest, AddComponentsBulkWithInvalidIDsReturnsNotFoundList)
{
    std::vector<EntityID> eids = {999, 1000};
    auto notFound = m_entityRegistry->addComponentsBulk<Velocity>(eids);

    EXPECT_EQ(notFound.size(), 2);
}

TEST_F(ECSTest, AddComponentsBulkWithUnregisteredComponentThrows)
{
    struct UnregisteredComponent
    {
        int data;
    };

    auto metas = m_entityRegistry->createEntityBulk<Position>(3);
    std::vector<EntityID> eids = {metas[0].id};

    EXPECT_THROW(m_entityRegistry->addComponentsBulk<UnregisteredComponent>(eids),
                 std::runtime_error);
}

TEST_F(ECSTest, RemoveComponentsBulkModifiesMultipleEntities)
{
    auto metas = m_entityRegistry->createEntityBulk<Position, Velocity, Health>(5);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    auto notFound = m_entityRegistry->removeComponentsBulk<Velocity>(eids);

    EXPECT_TRUE(notFound.empty());

    // Entities should now have only Position and Health
    auto result = m_entityRegistry->viewSet<Position, Health>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Health&) { count++; });

    EXPECT_EQ(count, 5);

    // Verify Velocity was removed
    auto velocityView = m_entityRegistry->viewSubset<Velocity>();
    EXPECT_FALSE(velocityView.has_value());
}

TEST_F(ECSTest, RemoveComponentsBulkWithEmptyVectorReturnsEmpty)
{
    m_entityRegistry->createEntityBulk<Position, Velocity>(3);

    std::vector<EntityID> empty;
    auto notFound = m_entityRegistry->removeComponentsBulk<Velocity>(empty);

    EXPECT_TRUE(notFound.empty());
}

TEST_F(ECSTest, RemoveComponentsBulkHandlesNonExistentComponentGracefully)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(3);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    // Attempt to remove component they don't have
    EXPECT_NO_THROW(m_entityRegistry->removeComponentsBulk<Velocity>(eids));

    // Should still have 3 entities with Position only
    EXPECT_EQ(m_entityRegistry->entityCount(), 3);
}

TEST_F(ECSTest, ModifyComponentsBulkAddsAndRemovesSimultaneously)
{
    auto metas = m_entityRegistry->createEntityBulk<Position, Velocity>(5);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    auto notFound = m_entityRegistry->modifyComponentsBulk(detail::Add<Health>{},
                                                           detail::Remove<Velocity>{}, eids);

    EXPECT_TRUE(notFound.empty());

    // Should now have Position + Health
    auto result = m_entityRegistry->viewSet<Position, Health>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Health&) { count++; });

    EXPECT_EQ(count, 5);
}

TEST_F(ECSTest, ModifyComponentsBulkWithEmptyAddAndRemoveIsNoOp)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(5);

    std::vector<EntityID> eids;
    for (const auto& meta : metas)
    {
        eids.push_back(meta.id);
    }

    auto notFound =
        m_entityRegistry->modifyComponentsBulk(detail::Add<>{}, detail::Remove<>{}, eids);

    EXPECT_TRUE(notFound.empty());

    // Entities should remain unchanged
    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&) { count++; });

    EXPECT_EQ(count, 5);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Archetype Migrations API Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, ModifyComponentsSingleEntityMigratesArchetype)
{
    auto meta = m_entityRegistry->createEntity<Position>().id;

    m_entityRegistry->modifyComponents(meta, detail::Add<Velocity>{}, detail::Remove<>{});

    // Entity should now be in archetype with Position + Velocity
    auto result = m_entityRegistry->getComponentsForEntity<Position, Velocity>(meta);
    ASSERT_TRUE(result.has_value());

    // Should have migrated from one archetype to another
    EXPECT_EQ(m_entityRegistry->archetypeCount(), 2);
}

TEST_F(ECSTest, ModifyComponentsSingleEntityRemovesComponentAndMigrates)
{
    auto meta = m_entityRegistry->createEntity<Position, Velocity>().id;

    m_entityRegistry->modifyComponents(meta, detail::Add<>{}, detail::Remove<Velocity>{});

    // Entity should now only have Position
    auto result = m_entityRegistry->getComponentsForEntity<Position>(meta);
    ASSERT_TRUE(result.has_value());

    auto velocityResult = m_entityRegistry->getComponentsForEntity<Velocity>(meta);
    EXPECT_FALSE(velocityResult.has_value());
}

TEST_F(ECSTest, ModifyComponentsSingleEntityPreservesExistingData)
{
    auto meta = m_entityRegistry->createEntity<Position>().id;

    // Set initial position
    {
        auto result = m_entityRegistry->getComponentsForEntity<Position>(meta);
        ASSERT_TRUE(result.has_value());
        auto& pos = std::get<0>(result.value());
        pos.x = 100.0f;
        pos.y = 200.0f;
        pos.z = 300.0f;
    }

    // Add Velocity component (triggers migration)
    m_entityRegistry->modifyComponents(meta, detail::Add<Velocity>{}, detail::Remove<>{});

    // Verify Position data survived migration
    {
        auto result = m_entityRegistry->getComponentsForEntity<Position>(meta);
        ASSERT_TRUE(result.has_value());
        auto& pos = std::get<0>(result.value());
        EXPECT_EQ(pos.x, 100.0f);
        EXPECT_EQ(pos.y, 200.0f);
        EXPECT_EQ(pos.z, 300.0f);
    }
}

TEST_F(ECSTest, ModifyComponentsSingleEntityWithNoSignatureChangeIsNoOp)
{
    auto meta = m_entityRegistry->createEntity<Position, Velocity>().id;

    // Attempt to add component already present
    m_entityRegistry->modifyComponents(meta, detail::Add<Position>{}, detail::Remove<>{});

    // Should still be in same archetype
    EXPECT_EQ(m_entityRegistry->archetypeCount(), 1);
    EXPECT_EQ(m_entityRegistry->entityCount(), 1);
}

TEST_F(ECSTest, ModifyComponentsSingleEntityWithInvalidIDIsNoOp)
{
    EntityID invalidID = 999;
    EXPECT_NO_THROW(
        m_entityRegistry->modifyComponents(invalidID, detail::Add<Velocity>{}, detail::Remove<>{}));
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsMovesAllEntities)
{
    // Create source archetype with Position only
    m_entityRegistry->createEntityBulk<Position>(10);

    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position>{}, detail::Add<Velocity>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 10);

    // All entities should now be in archetype with Position + Velocity
    auto result = m_entityRegistry->viewSet<Position, Velocity>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Velocity&) { count++; });

    EXPECT_EQ(count, 10);

    // Source archetype should now be empty
    auto sourceView = m_entityRegistry->viewSet<Position>();
    EXPECT_FALSE(sourceView.has_value());
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsRemovesComponents)
{
    // Create source archetype with Position + Velocity
    m_entityRegistry->createEntityBulk<Position, Velocity>(5);

    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position, Velocity>{}, detail::Add<>{}, detail::Remove<Velocity>{});

    EXPECT_EQ(migratedCount, 5);

    // All entities should now have only Position
    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&) { count++; });

    EXPECT_EQ(count, 5);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsAddsAndRemovesSimultaneously)
{
    m_entityRegistry->createEntityBulk<Position, Velocity>(7);

    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position, Velocity>{}, detail::Add<Health>{}, detail::Remove<Velocity>{});

    EXPECT_EQ(migratedCount, 7);

    // Should have Position + Health
    auto result = m_entityRegistry->viewSet<Position, Health>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Health&) { count++; });

    EXPECT_EQ(count, 7);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsPreservesExistingData)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(3);

    // Set unique positions
    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());
    result.value().forEach(
        [i = 0.0f](EntityMeta, Position& pos) mutable
        {
            pos.x = i++;
            pos.y = i * 2.0f;
            pos.z = i * 3.0f;
        });

    // Migrate and add Velocity
    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position>{}, detail::Add<Velocity>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 3);

    // Verify Position data survived
    auto newResult = m_entityRegistry->viewSet<Position, Velocity>();
    ASSERT_TRUE(newResult.has_value());

    std::vector<float> xValues;
    newResult.value().forEach([&](EntityMeta, Position& pos, Velocity&)
                              { xValues.push_back(pos.x); });

    EXPECT_EQ(xValues.size(), 3);
    // Values should be preserved (0.0, 1.0, 2.0)
    EXPECT_FLOAT_EQ(xValues[0], 0.0f);
    EXPECT_FLOAT_EQ(xValues[1], 1.0f);
    EXPECT_FLOAT_EQ(xValues[2], 2.0f);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsWithNonExistentSourceReturnsZero)
{
    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position, Velocity>{}, detail::Add<Health>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 0);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsWithEmptySourceReturnsZero)
{
    // Create then destroy all entities
    auto metas = m_entityRegistry->createEntityBulk<Position>(5);
    std::vector<EntityID> eids;
    for (auto& m : metas) eids.push_back(m.id);
    m_entityRegistry->destroyEntityBulk(eids);

    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position>{}, detail::Add<Velocity>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 0);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsWithUnregisteredComponentThrows)
{
    struct UnregisteredComponent
    {
        int data;
    };

    m_entityRegistry->createEntityBulk<Position>(3);

    EXPECT_THROW(
        m_entityRegistry->migrateArchetypeModifyComponents(
            detail::From<Position>{}, detail::Add<UnregisteredComponent>{}, detail::Remove<>{}),
        std::runtime_error);
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsWithNoSignatureChangeReturnsZero)
{
    m_entityRegistry->createEntityBulk<Position>(5);

    // Attempt migration with no actual changes
    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position>{}, detail::Add<>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 0);

    // Entities should remain unchanged
    auto result = m_entityRegistry->viewSet<Position>();
    ASSERT_TRUE(result.has_value());
}

TEST_F(ECSTest, MigrateArchetypeModifyComponentsHandlesMultipleComponentAddition)
{
    m_entityRegistry->createEntityBulk<Position>(4);

    size_t migratedCount = m_entityRegistry->migrateArchetypeModifyComponents(
        detail::From<Position>{}, detail::Add<Velocity, Health>{}, detail::Remove<>{});

    EXPECT_EQ(migratedCount, 4);

    // Should have all three components
    auto result = m_entityRegistry->viewSet<Position, Velocity, Health>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Velocity&, Health&) { count++; });

    EXPECT_EQ(count, 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Edge Cases for Bulk/Migration Operations
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, BulkOperationsHandleLargeEntityCounts)
{
    // Create 1000 entities
    auto metas = m_entityRegistry->createEntityBulk<Position>(1000);
    EXPECT_EQ(metas.size(), 1000);
    EXPECT_EQ(m_entityRegistry->entityCount(), 1000);

    // Bulk add components
    std::vector<EntityID> eids;
    for (auto& m : metas) eids.push_back(m.id);

    auto notFound = m_entityRegistry->addComponentsBulk<Velocity>(eids);
    EXPECT_TRUE(notFound.empty());

    // Verify all migrated
    auto result = m_entityRegistry->viewSet<Position, Velocity>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Velocity&) { count++; });
    EXPECT_EQ(count, 1000);
}

TEST_F(ECSTest, MigrationPreservesEntityIDAndGeneration)
{
    auto meta = m_entityRegistry->createEntity<Position>();

    EntityID originalID = meta.id;
    EntityGen originalGen = meta.gen;

    // Trigger migration
    m_entityRegistry->addComponents<Velocity>(meta.id);

    // Verify entity is still valid with same ID/generation
    EXPECT_TRUE(m_entityRegistry->isEntityValid(meta));

    auto arch = m_entityRegistry->getArchetypeForEntity(originalID);
    ASSERT_NE(arch, nullptr);

    uint8_t* rowPtr = arch->get(originalID);
    ASSERT_NE(rowPtr, nullptr);

    EntityMeta* storedMeta = reinterpret_cast<EntityMeta*>(rowPtr);
    EXPECT_EQ(storedMeta->id, originalID);
    EXPECT_EQ(storedMeta->gen, originalGen);
}

TEST_F(ECSTest, ConsecutiveBulkOperationsOnSameEntities)
{
    auto metas = m_entityRegistry->createEntityBulk<Position>(10);
    std::vector<EntityID> eids;
    for (auto& m : metas) eids.push_back(m.id);

    // Add Velocity
    m_entityRegistry->addComponentsBulk<Velocity>(eids);

    // Add Health
    m_entityRegistry->addComponentsBulk<Health>(eids);

    // Remove Velocity
    m_entityRegistry->removeComponentsBulk<Velocity>(eids);

    // Should end up with Position + Health
    auto result = m_entityRegistry->viewSet<Position, Health>();
    ASSERT_TRUE(result.has_value());

    int count = 0;
    result.value().forEach([&](EntityMeta, Position&, Health&) { count++; });
    EXPECT_EQ(count, 10);
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor Arguments Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ECSTest, CreateEntityWithConstructorArgs)
{
    // Create entity with constructor args
    auto meta = m_entityRegistry->createEntity<Position, Velocity>(
        std::make_tuple(10.0f, 20.0f, 30.0f), // Position(x, y, z)
        std::make_tuple(1.0f, 2.0f, 3.0f)     // Velocity(dx, dy, dz)
    );

    // Verify values were initialized correctly
    auto result = m_entityRegistry->getComponentsForEntity<Position, Velocity>(meta.id);
    ASSERT_TRUE(result.has_value());

    auto& [pos, vel] = result.value();
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);
    EXPECT_FLOAT_EQ(pos.z, 30.0f);
    EXPECT_FLOAT_EQ(vel.dx, 1.0f);
    EXPECT_FLOAT_EQ(vel.dy, 2.0f);
    EXPECT_FLOAT_EQ(vel.dz, 3.0f);
}

TEST_F(ECSTest, CreateEntityWithSingleComponentConstructorArgs)
{
    // Create entity with single component and args
    auto meta =
        m_entityRegistry->createEntity<Health>(std::make_tuple(100, 100) // Health(hp, maxHp)
        );

    auto result = m_entityRegistry->getComponentsForEntity<Health>(meta.id);
    ASSERT_TRUE(result.has_value());

    auto& health = std::get<0>(result.value());
    EXPECT_EQ(health.hp, 100);
    EXPECT_EQ(health.maxHp, 100);
}

TEST_F(ECSTest, CreateEntityBulkWithSharedArgs)
{
    // Create 100 entities with same constructor args
    auto metas = m_entityRegistry->createEntityBulk<Position>(
        100, std::make_tuple(5.0f, 5.0f, 5.0f) // All entities get Position(5, 5, 5)
    );

    ASSERT_EQ(metas.size(), 100);

    // Verify all have same values
    for (const auto& meta : metas)
    {
        auto result = m_entityRegistry->getComponentsForEntity<Position>(meta.id);
        ASSERT_TRUE(result.has_value());

        auto& pos = std::get<0>(result.value());
        EXPECT_FLOAT_EQ(pos.x, 5.0f);
        EXPECT_FLOAT_EQ(pos.y, 5.0f);
        EXPECT_FLOAT_EQ(pos.z, 5.0f);
    }
}

TEST_F(ECSTest, CreateEntityBulkWithMultipleComponentsAndSharedArgs)
{
    // Create entities with multiple components and shared args
    auto metas = m_entityRegistry->createEntityBulk<Position, Health>(
        50, std::make_tuple(0.0f, 0.0f, 0.0f), // Position(0, 0, 0)
        std::make_tuple(50, 100)               // Health(50, 100)
    );

    ASSERT_EQ(metas.size(), 50);

    // Verify first and last have correct values
    auto first = m_entityRegistry->getComponentsForEntity<Position, Health>(metas[0].id);
    ASSERT_TRUE(first.has_value());
    EXPECT_FLOAT_EQ(std::get<0>(first.value()).x, 0.0f);
    EXPECT_EQ(std::get<1>(first.value()).hp, 50);
    EXPECT_EQ(std::get<1>(first.value()).maxHp, 100);

    auto last = m_entityRegistry->getComponentsForEntity<Position, Health>(metas[49].id);
    ASSERT_TRUE(last.has_value());
    EXPECT_FLOAT_EQ(std::get<0>(last.value()).x, 0.0f);
    EXPECT_EQ(std::get<1>(last.value()).hp, 50);
}

TEST_F(ECSTest, AddComponentsWithArgs)
{
    // Create entity with Position
    auto meta = m_entityRegistry->createEntity<Position>(std::make_tuple(1.0f, 2.0f, 3.0f));

    // Add Velocity with args (triggers archetype migration)
    m_entityRegistry->addComponents<Velocity>(meta.id, std::make_tuple(10.0f, 20.0f, 30.0f));

    // Verify both components
    auto result = m_entityRegistry->getComponentsForEntity<Position, Velocity>(meta.id);
    ASSERT_TRUE(result.has_value());

    auto& [pos, vel] = result.value();
    EXPECT_FLOAT_EQ(pos.x, 1.0f); // Original Position preserved
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
    EXPECT_FLOAT_EQ(vel.dx, 10.0f); // New Velocity has args
    EXPECT_FLOAT_EQ(vel.dy, 20.0f);
    EXPECT_FLOAT_EQ(vel.dz, 30.0f);
}

TEST_F(ECSTest, AddMultipleComponentsWithArgs)
{
    // Create entity with just Position
    auto meta = m_entityRegistry->createEntity<Position>(std::make_tuple(5.0f, 10.0f, 15.0f));

    // Add Velocity and Health with args
    m_entityRegistry->modifyComponents(meta.id, detail::Add<Velocity, Health>{}, detail::Remove<>{},
                                       std::make_tuple(1.0f, 2.0f, 3.0f), // Velocity args
                                       std::make_tuple(75, 100)           // Health args
    );

    // Verify all three components
    auto result = m_entityRegistry->getComponentsForEntity<Position, Velocity, Health>(meta.id);
    ASSERT_TRUE(result.has_value());

    auto& [pos, vel, health] = result.value();
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
    EXPECT_FLOAT_EQ(vel.dx, 1.0f);
    EXPECT_EQ(health.hp, 75);
    EXPECT_EQ(health.maxHp, 100);
}

TEST_F(ECSTest, MixedConstructorArgsAndDefaultConstruction)
{
    // Create entity with args
    auto meta1 = m_entityRegistry->createEntity<Position>(std::make_tuple(1.0f, 2.0f, 3.0f));

    // Create entity without args
    auto meta2 = m_entityRegistry->createEntity<Position>();

    // Both should exist in same archetype
    EXPECT_EQ(m_entityRegistry->archetypeCount(), 1);
    EXPECT_EQ(m_entityRegistry->entityCount(), 2);

    // Verify first has initialized values
    auto result1 = m_entityRegistry->getComponentsForEntity<Position>(meta1.id);
    ASSERT_TRUE(result1.has_value());
    EXPECT_FLOAT_EQ(std::get<0>(result1.value()).x, 1.0f);

    // Second exists but values are default-initialized (indeterminate)
    auto result2 = m_entityRegistry->getComponentsForEntity<Position>(meta2.id);
    EXPECT_TRUE(result2.has_value());
}
