#include <benchmark/benchmark.h>

#include <unordered_map>

#include <pieces/containers/sparse_set.hpp>

using namespace pieces;

static void BM_SparseSet_Insert(benchmark::State& state)
{
    for (auto _ : state)
    {
        SparseSet<uint32_t, int> set;
        for (uint32_t i = 0; i < state.range(0); ++i) set.insert(i, static_cast<int>(i));
    }
}

BENCHMARK(BM_SparseSet_Insert)->Range(1 << 10, 1 << 20);

static void BM_UnorderedMap_Insert(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::unordered_map<uint32_t, int> map;
        for (uint32_t i = 0; i < state.range(0); ++i) map.emplace(i, static_cast<int>(i));
    }
}

BENCHMARK(BM_UnorderedMap_Insert)->Range(1 << 10, 1 << 20);

template <int PageSize>
static void BM_SparseSet_Insert_PS(benchmark::State& state)
{
    for (auto _ : state)
    {
        SparseSet<uint32_t, int, PageSize> set;
        for (uint32_t i = 0; i < state.range(0); ++i) set.insert(i, static_cast<int>(i));
    }
}

BENCHMARK_TEMPLATE(BM_SparseSet_Insert_PS, 32)->Range(1 << 10, 1 << 20);
BENCHMARK_TEMPLATE(BM_SparseSet_Insert_PS, 64)->Range(1 << 10, 1 << 20);
BENCHMARK_TEMPLATE(BM_SparseSet_Insert_PS, 128)->Range(1 << 10, 1 << 20);
BENCHMARK_TEMPLATE(BM_SparseSet_Insert_PS, 256)->Range(1 << 10, 1 << 20);

static void BM_SparseSet_Iterate(benchmark::State& state)
{
    SparseSet<uint32_t, int> set;
    for (uint32_t i = 0; i < state.range(0); ++i) set.insert(i, static_cast<int>(i));

    for (auto _ : state)
    {
        int sum = 0;
        for (const auto& value : set.values()) sum += value;

        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_SparseSet_Iterate)->Range(1 << 10, 1 << 20);

static void BM_UnorderedMap_Iterate(benchmark::State& state)
{
    std::unordered_map<uint32_t, int> map;
    for (uint32_t i = 0; i < state.range(0); ++i) map.emplace(i, static_cast<int>(i));

    for (auto _ : state)
    {
        int sum = 0;
        for (const auto& [k, v] : map) sum += v;

        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_UnorderedMap_Iterate)->Range(1 << 10, 1 << 20);
