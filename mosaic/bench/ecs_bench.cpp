#include <benchmark/benchmark.h>

#include <random>
#include <memory>
#include <vector>
#include <algorithm>

#include <saturn/ecs/entity_registry.hpp>

using namespace saturn::ecs;

// Test components
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

struct Transform
{
    float matrix[16];
};

struct Renderable
{
    int meshId, textureId;
};

static std::unique_ptr<ComponentRegistry> g_componentRegistry = nullptr;
static std::unique_ptr<EntityRegistry> g_entityRegistry = nullptr;

// Benchmark fixture
class ECSBenchmark : public benchmark::Fixture
{
   public:
    void SetUp(const ::benchmark::State& state) override
    {
        if (!g_componentRegistry)
        {
            g_componentRegistry = std::make_unique<ComponentRegistry>(64);

            g_componentRegistry->registerComponent<Position>("Position");
            g_componentRegistry->registerComponent<Velocity>("Velocity");
            g_componentRegistry->registerComponent<Health>("Health");
            g_componentRegistry->registerComponent<Transform>("Transform");
            g_componentRegistry->registerComponent<Renderable>("Renderable");
        }

        if (!g_entityRegistry)
        {
            g_entityRegistry = std::make_unique<EntityRegistry>(g_componentRegistry.get());
        }
        else
        {
            g_entityRegistry->clear();
        }
    }
};

BENCHMARK_DEFINE_F(ECSBenchmark, EntityCreation_SingleComponent)(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto eid = g_entityRegistry->createEntity<Position>();
        benchmark::DoNotOptimize(eid);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["entities"] = g_entityRegistry->entityCount();
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

BENCHMARK_DEFINE_F(ECSBenchmark, EntityCreation_ThreeComponents)(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto eid = g_entityRegistry->createEntity<Position, Velocity, Health>();
        benchmark::DoNotOptimize(eid);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["entities"] = g_entityRegistry->entityCount();
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

BENCHMARK_DEFINE_F(ECSBenchmark, EntityCreation_FiveComponents)(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto eid =
            g_entityRegistry->createEntity<Position, Velocity, Health, Transform, Renderable>();
        benchmark::DoNotOptimize(eid);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["entities"] = g_entityRegistry->entityCount();
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

BENCHMARK_DEFINE_F(ECSBenchmark, BatchEntityCreation)(benchmark::State& state)
{
    const int batch_size = state.range(0);

    for (auto _ : state)
    {
        std::vector<EntityID> entities;
        entities.reserve(batch_size);

        for (int i = 0; i < batch_size; ++i)
        {
            entities.push_back(g_entityRegistry->createEntity<Position, Velocity>().id);
        }

        benchmark::DoNotOptimize(entities);

        // Clean up for next iteration
        g_entityRegistry->destroyEntities(entities);
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK_DEFINE_F(ECSBenchmark, EntityDestruction)(benchmark::State& state)
{
    // Pre-create entities
    std::vector<EntityID> entities;
    const int entity_count = 10000;
    entities.reserve(entity_count);

    for (int i = 0; i < entity_count; ++i)
    {
        entities.push_back(g_entityRegistry->createEntity<Position, Velocity>().id);
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    for (auto _ : state)
    {
        state.PauseTiming();

        // Shuffle for random access pattern
        std::shuffle(entities.begin(), entities.end(), gen);
        auto eid = entities.back();
        entities.pop_back();

        state.ResumeTiming();

        g_entityRegistry->destroyEntity(eid);

        // Replenish if running low
        if (entities.size() < 1000)
        {
            state.PauseTiming();
            for (int i = 0; i < 1000; ++i)
            {
                entities.push_back(g_entityRegistry->createEntity<Position, Velocity>().id);
            }
            state.ResumeTiming();
        }
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(ECSBenchmark, ComponentAddition)(benchmark::State& state)
{
    // Pre-create entities with single component
    std::vector<EntityID> entities;
    const int entity_count = 1000;

    for (int i = 0; i < entity_count; ++i)
    {
        entities.push_back(g_entityRegistry->createEntity<Position>().id);
    }

    int idx = 0;
    for (auto _ : state)
    {
        g_entityRegistry->addComponents<Velocity>(entities[idx % entity_count]);
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

BENCHMARK_DEFINE_F(ECSBenchmark, ComponentRemoval)(benchmark::State& state)
{
    // Pre-create entities with multiple components
    std::vector<EntityID> entities;
    const int entity_count = 1000;

    for (int i = 0; i < entity_count; ++i)
    {
        entities.push_back(g_entityRegistry->createEntity<Position, Velocity, Health>().id);
    }

    int idx = 0;
    for (auto _ : state)
    {
        g_entityRegistry->removeComponents<Velocity>(entities[idx % entity_count]);
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSetIteration_SingleComponent)(benchmark::State& state)
{
    const int entity_count = state.range(0);

    // Create entities
    for (int i = 0; i < entity_count; ++i)
    {
        g_entityRegistry->createEntity<Position>();
    }

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSet<Position>();
        if (view)
        {
            view->forEach(
                [](EntityMeta meta, Position& pos)
                {
                    pos.x += 1.0f; // Simulate work
                    benchmark::DoNotOptimize(pos);
                });
        }
    }

    state.SetItemsProcessed(state.iterations() * entity_count);
    state.SetBytesProcessed(state.iterations() * entity_count * sizeof(Position));
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSetIteration_MultipleComponents)(benchmark::State& state)
{
    const int entity_count = state.range(0);

    // Create entities
    for (int i = 0; i < entity_count; ++i)
    {
        g_entityRegistry->createEntity<Position, Velocity>();
    }

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSet<Position, Velocity>();
        if (view)
        {
            view->forEach(
                [](EntityMeta meta, Position& pos, Velocity& vel)
                {
                    pos.x += vel.dx; // Simulate work
                    pos.y += vel.dy;
                    pos.z += vel.dz;
                    benchmark::DoNotOptimize(pos);
                });
        }
    }

    state.SetItemsProcessed(state.iterations() * entity_count);
    state.SetBytesProcessed(state.iterations() * entity_count *
                            (sizeof(Position) + sizeof(Velocity)));
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSetIteration_Iterator)(benchmark::State& state)
{
    const int entity_count = state.range(0);

    // Create entities
    for (int i = 0; i < entity_count; ++i)
    {
        g_entityRegistry->createEntity<Position, Velocity>();
    }

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSet<Position, Velocity>();
        if (view)
        {
            for (auto [meta, components] : *view)
            {
                auto& [pos, vel] = components;
                pos.x += vel.dx; // Simulate work
                benchmark::DoNotOptimize(pos);
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * entity_count);
}

void createEntitiesWithCompsCombinations(int _count)
{
    const int entityCount = _count;

    for (int i = 0; i < entityCount / 3; ++i)
    {
        g_entityRegistry->createEntity<Position, Velocity>();
    }
    for (int i = 0; i < entityCount / 3; ++i)
    {
        g_entityRegistry->createEntity<Position, Velocity, Health>();
    }
    for (int i = 0; i < entityCount / 3; ++i)
    {
        g_entityRegistry->createEntity<Position, Velocity, Health, Transform>();
    }
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSubsetIteration_SingleComponent)(benchmark::State& state)
{
    const int entityCount = state.range(0);

    createEntitiesWithCompsCombinations(entityCount);

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSubset<Position>();
        if (view)
        {
            view->forEach(
                [](EntityMeta meta, Position& pos)
                {
                    pos.x += 1.0f; // Simulate work
                    benchmark::DoNotOptimize(pos);
                });
        }
    }

    state.SetItemsProcessed(state.iterations() * entityCount);
    state.SetBytesProcessed(state.iterations() * entityCount * sizeof(Position));
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSubsetIteration_MultipleComponents)(benchmark::State& state)
{
    const int entityCount = state.range(0);

    createEntitiesWithCompsCombinations(entityCount);

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSubset<Position, Velocity>();
        if (view)
        {
            view->forEach(
                [](EntityMeta meta, Position& pos, Velocity& vel)
                {
                    pos.x += vel.dx; // Simulate work
                    pos.y += vel.dy;
                    pos.z += vel.dz;
                    benchmark::DoNotOptimize(pos);
                });
        }
    }

    state.SetItemsProcessed(state.iterations() * entityCount);
    state.SetBytesProcessed(state.iterations() * entityCount *
                            (sizeof(Position) + sizeof(Velocity)));
}

BENCHMARK_DEFINE_F(ECSBenchmark, ViewSubsetIteration_Iterator)(benchmark::State& state)
{
    const int entityCount = state.range(0);

    createEntitiesWithCompsCombinations(entityCount);

    for (auto _ : state)
    {
        auto view = g_entityRegistry->viewSubset<Position, Velocity>();
        if (view)
        {
            for (auto [meta, components] : *view)
            {
                auto& [pos, vel] = components;
                pos.x += vel.dx; // Simulate work
                benchmark::DoNotOptimize(pos);
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * entityCount);
}

// Stress test - mixed operations
BENCHMARK_DEFINE_F(ECSBenchmark, MixedOperations)(benchmark::State& state)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> op_dist(0, 4);

    std::vector<EntityID> entities;
    entities.reserve(1000);

    for (auto _ : state)
    {
        int operation = op_dist(gen);

        switch (operation)
        {
            case 0: // Create entity
                if (entities.size() < 1000)
                {
                    entities.push_back(g_entityRegistry->createEntity<Position, Velocity>().id);
                }
                break;

            case 1: // Destroy entity
                if (!entities.empty())
                {
                    auto it = entities.begin() + (gen() % entities.size());
                    g_entityRegistry->destroyEntity(*it);
                    entities.erase(it);
                }
                break;

            case 2: // Add component
                if (!entities.empty())
                {
                    auto eid = entities[gen() % entities.size()];
                    g_entityRegistry->addComponents<Health>(eid);
                }
                break;

            case 3: // Remove component
                if (!entities.empty())
                {
                    auto eid = entities[gen() % entities.size()];
                    g_entityRegistry->removeComponents<Velocity>(eid);
                }
                break;

            case 4: // Iterate
            {
                auto view = g_entityRegistry->viewSet<Position>();
                if (view)
                {
                    view->forEach(
                        [](EntityMeta meta, Position& pos)
                        {
                            pos.x += 0.1f;
                            benchmark::DoNotOptimize(pos);
                        });
                }
            }
            break;
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["entities"] = entities.size();
    state.counters["archetypes"] = g_entityRegistry->archetypeCount();
}

// Register benchmarks
BENCHMARK_REGISTER_F(ECSBenchmark, EntityCreation_SingleComponent)->Unit(benchmark::kNanosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, EntityCreation_ThreeComponents)->Unit(benchmark::kNanosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, EntityCreation_FiveComponents)->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, BatchEntityCreation)
    ->Range(10, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, EntityDestruction)->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, ComponentAddition)->Unit(benchmark::kNanosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, ComponentRemoval)->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, ViewSetIteration_SingleComponent)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, ViewSetIteration_MultipleComponents)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, ViewSetIteration_Iterator)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, ViewSubsetIteration_SingleComponent)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, ViewSubsetIteration_MultipleComponents)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_REGISTER_F(ECSBenchmark, ViewSubsetIteration_Iterator)
    ->Range(100, 100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(ECSBenchmark, MixedOperations)
    ->Iterations(10000)
    ->Unit(benchmark::kNanosecond);
