#include <benchmark/benchmark.h>

#include <zx/sequence.hpp>
#include <vector>
#include <functional>

namespace zx::bench
{

// ============================================================================
// Type-erased (std::function-based) sequence implementations
// ============================================================================

template <class T>
using erased_next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
using erased_sequence_t = sequence_t<T, erased_next_function_t<T>>;

// ============================================================================
// Benchmark: Simple range creation and iteration
// ============================================================================

static void BM_Erased_RangeIteration(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

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

// ============================================================================
// Benchmark: Transform operation
// ============================================================================

static void BM_Erased_Transform(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        // Wrap the transformed sequence to be type-erased
        auto transformed = erased_sequence_t<int>([seq_func = seq.get_next_function()]() mutable -> iteration_result_t<int> {
            auto val = seq_func();
            if (val) return *val * 2;
            return {};
        });

        int sum = 0;
        for (auto val : transformed)
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

// ============================================================================
// Benchmark: Filter operation
// ============================================================================

static void BM_Erased_Filter(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        // Filter for even numbers
        auto filtered = erased_sequence_t<int>([seq_func = seq.get_next_function()]() mutable -> iteration_result_t<int> {
            while (true)
            {
                auto val = seq_func();
                if (!val) return {};
                if (*val % 2 == 0) return val;
            }
        });

        int sum = 0;
        for (auto val : filtered)
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

// ============================================================================
// Benchmark: Chained operations (transform + filter + transform)
// ============================================================================

static void BM_Erased_Chained(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        // filter even -> multiply by 2 -> add 1
        // This is complex with type-erased, so we do a simplified version
        auto result = erased_sequence_t<int>([seq_func = seq.get_next_function()]() mutable -> iteration_result_t<int> {
            while (true)
            {
                auto val = seq_func();
                if (!val) return {};
                if (*val % 2 == 0) return (*val * 2) + 1;
            }
        });

        int sum = 0;
        for (auto val : result)
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

// ============================================================================
// Benchmark: Drop and Take operations
// ============================================================================

static void BM_Erased_DropTake(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        // Simplified: drop first 10, take next 10
        auto result = erased_sequence_t<int>([seq_func = seq.get_next_function(), skip = 10, take = 10, count = 0]() mutable -> iteration_result_t<int> {
            while (count < skip)
            {
                seq_func();
                count++;
            }

            if (take <= 0)
                return {};

            auto val = seq_func();
            if (val)
            {
                take--;
                return val;
            }
            return {};
        });

        int sum = 0;
        for (auto val : result)
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

// ============================================================================
// Benchmark: Step operation
// ============================================================================

static void BM_Erased_Step(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        // Every 2nd element
        auto stepped = erased_sequence_t<int>([seq_func = seq.get_next_function(), idx = 0]() mutable -> iteration_result_t<int> {
            while (true)
            {
                auto val = seq_func();
                if (!val) return {};
                if (idx++ % 2 == 0) return val;
            }
        });

        int sum = 0;
        for (auto val : stepped)
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

// ============================================================================
// Benchmark: Fold operation (for_each)
// ============================================================================

static void BM_Erased_Fold(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = erased_sequence_t<int>([i = 0, limit = static_cast<int>(n)]() mutable -> iteration_result_t<int> {
            if (i >= limit) return {};
            return i++;
        });

        int result = 0;
        seq.for_each([&result](int x) { result += x; });
        benchmark::DoNotOptimize(result);
    }
}

static void BM_Template_Fold(benchmark::State& state)
{
    const auto n = state.range(0);
    for (auto _ : state)
    {
        auto seq = zx::seq::range(0, static_cast<int>(n));

        int result = 0;
        seq.for_each([&result](int x) { result += x; });
        benchmark::DoNotOptimize(result);
    }
}

// ============================================================================
// Register benchmarks
// ============================================================================

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
