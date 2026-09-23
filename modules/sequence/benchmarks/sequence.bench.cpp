#include <benchmark/benchmark.h>

#include <functional>
#include <vector>
#include <zx/sequence.hpp>

namespace zx::bench
{

static void BM_Erased_RangeIteration(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = sequence_t<int>(zx::seq::range(0, static_cast<int>(n)));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_RangeIteration(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_Transform(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::sequence_t<int>(zx::seq::range(0, static_cast<int>(n)).transform([](int x) { return x * 2; }));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_Transform(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n)).transform([](int x) { return x * 2; });

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_Filter(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::sequence_t<int>(zx::seq::range(0, static_cast<int>(n)).filter([](int x) { return x % 2 == 0; }));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_Filter(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n)).filter([](int x) { return x % 2 == 0; });

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_Chained(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::sequence_t<int>(zx::seq::range(0, static_cast<int>(n))
                                           .filter([](int x) { return x % 2 == 0; })
                                           .transform([](int x) { return x * 2; })
                                           .transform([](int x) { return x + 1; }));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_Chained(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n))
                       .filter([](int x) { return x % 2 == 0; })
                       .transform([](int x) { return x * 2; })
                       .transform([](int x) { return x + 1; });

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_DropTake(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::sequence_t<int>(zx::seq::range(0, static_cast<int>(n)).drop(10).take(10));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_DropTake(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n)).drop(10).take(10);

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_Step(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::sequence_t<int>(zx::seq::range(0, static_cast<int>(n)).step(2));

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Template_Step(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n)).step(2);

        int sum = 0;
        for (auto val : seq)
        {
            sum += val;
            benchmark::DoNotOptimize(sum);
        }
    }
}

static void BM_Erased_Fold(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = sequence_t<int>(zx::seq::range(0, static_cast<int>(n)));

        int result = 0;
        seq.for_each(
            [&result](int x)
            {
                result += x;
                benchmark::DoNotOptimize(result);
            });
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_Template_Fold(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n));

        int result = 0;
        seq.for_each(
            [&result](int x)
            {
                result += x;
                benchmark::DoNotOptimize(result);
            });
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_Erased_RangeIteration)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_RangeIteration)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_Transform)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_Transform)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_Filter)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_Filter)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_Chained)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_Chained)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_DropTake)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_DropTake)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_Step)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_Step)->Range(100, 100000)->UseRealTime();

BENCHMARK(BM_Erased_Fold)->Range(100, 100000)->UseRealTime();
BENCHMARK(BM_Template_Fold)->Range(100, 100000)->UseRealTime();

}  // namespace zx::bench

BENCHMARK_MAIN();
