# Sequence Benchmarks

This directory contains Google Benchmark-based performance benchmarks for the `sequence_t` template-based implementation.

## Overview

The benchmarks compare the performance of two approaches:

1. **Type-Erased Approach** (`zx::sequence_t<T, std::function<...>>`)
   - Uses `std::function` to type-erase the `next_function_type`
   - Name pattern: `BM_Erased_*`

2. **Template-Based Approach** (non-type-erased)
   - Keeps the `NextFn` type as a template parameter in `sequence_t<T, NextFn>`
   - Allows for zero-cost abstractions and inline optimization
   - Name pattern: `BM_Template_*`

## Building the Benchmarks

### Prerequisites

- CMake 3.14 or later
- C++17 compliant compiler
- Google Benchmark (automatically downloaded via FetchContent)

### Build Instructions

To build the benchmarks with Release optimizations:

```bash
cd /home/krzysiek/projects/zx
cmake -B build \
  -DZX_BUILD_SEQUENCE=ON \
  -DZX_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target sequence_benchmarks -j4
```

Or use the simpler approach with the existing build:

```bash
cd /home/krzysiek/projects/zx/build
cmake .. -DZX_BUILD_BENCHMARKS=ON -DZX_BUILD_SEQUENCE=ON -DCMAKE_BUILD_TYPE=Release
make sequence_benchmarks -j4
```

### Running the Benchmarks

```bash
# Run all benchmarks with default settings
./build/modules/sequence/sequence_benchmarks

# Run benchmarks with custom options
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Template --benchmark_time_unit=us

# Export results to JSON for comparison
./build/modules/sequence/sequence_benchmarks --benchmark_out=results.json --benchmark_out_format=json

# Compare results
./build/modules/sequence/sequence_benchmarks --benchmark_out=template.json --benchmark_filter=BM_Template
./build/modules/sequence/sequence_benchmarks --benchmark_out=erased.json --benchmark_filter=BM_Erased
```

## Benchmark Descriptions

### 1. RangeIteration
- **Purpose**: Measure basic sequence creation and iteration overhead
- **Operation**: Create a sequence from 0 to N, iterate and sum all elements
- **Metrics**: Pure iteration cost, minimal transformation

### 2. Transform
- **Purpose**: Measure the overhead of applying a transformation function
- **Operation**: Create sequence 0..N, transform each element (multiply by 2), sum results
- **Metrics**: Function call overhead, inlining capability

### 3. Filter
- **Purpose**: Measure filtering operation overhead
- **Operation**: Create sequence 0..N, filter for even numbers, sum results
- **Metrics**: Predicate evaluation, control flow

### 4. Chained
- **Purpose**: Measure composition of multiple operations
- **Operation**: Create 0..N → filter (even) → transform (×2) → transform (+1) → sum
- **Metrics**: Composition overhead, potential optimization opportunities

### 5. DropTake
- **Purpose**: Measure stateful operations
- **Operation**: Create 0..N, drop first 10 elements, take next 10, sum
- **Metrics**: State management, element skipping

### 6. Step
- **Purpose**: Measure strided iteration
- **Operation**: Create 0..N, take every 2nd element, sum
- **Metrics**: Index tracking, modulo operations

### 7. Fold (for_each)
- **Purpose**: Measure final reduction operation
- **Operation**: Create 0..N, iterate with a fold operation to sum
- **Metrics**: Lambda capture, iteration finalization

## Expected Results

The template-based approach (`BM_Template_*`) should generally outperform the type-erased approach (`BM_Erased_*`) for several reasons:

1. **Inlining**: Template-based next functions can be inlined completely
2. **Specialization**: Each operation specializes to its specific types
3. **Zero-Cost Abstraction**: No virtual function calls or runtime type information
4. **Branch Prediction**: Compiler has full visibility for optimization

However, the degree of improvement varies:
- **Simple operations**: Significant gains (2-5x) due to inlining
- **Chained operations**: Diminishing returns as work dominates
- **Large iterations**: Gap narrows as iteration cost dominates

## Benchmark Parameters

Each benchmark runs over a range of input sizes:
- **Range**: 100 to 100,000 elements
- **Unit**: Real time (wall-clock) measurements
- **Repetitions**: Automatic (until stable)

## Tips for Benchmarking

1. **Disable turbo boost** for consistent results:
   ```bash
   echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
   ```

2. **Close background processes** to reduce noise

3. **Run multiple times** to verify consistency:
   ```bash
   for i in {1..3}; do ./build/modules/sequence/sequence_benchmarks --benchmark_out=run$i.json --benchmark_out_format=json; done
   ```

4. **Use `--benchmark_filter`** to focus on specific benchmarks

5. **Check CPU affinity** if running on multi-core systems

## Analyzing Results

### Key Metrics

- **Min/Max**: Shows consistency of measurements
- **Mean**: Average execution time
- **StdDev**: Standard deviation (lower is better for consistency)
- **Time/Unit**: Relative performance per element

### Interpretation

```
Benchmark                   Time    CPU    Iterations
BM_Template_RangeIteration  100 ns  100 ns  12345678  (fast - optimal)
BM_Erased_RangeIteration    200 ns  200 ns  6234567   (slow - type erasure cost)
```

The erased version takes ~2x the time, showing the overhead of virtual function calls through `std::function`.

## Further Optimization Ideas

1. **Small Function Optimization (SFO)**: Use fixed-size storage in sequence_t for small lambdas
2. **Lazy Evaluation**: Some operations don't need to be eagerly composed
3. **Vectorization**: SIMD optimizations for element-wise operations
4. **Specialization**: Explicit specializations for common operation chains
5. **Move Semantics**: Better move support to reduce copies in chained operations

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [C++ Performance Optimization](https://en.wikipedia.org/wiki/Optimization_(computer_science))
- [Zero-Cost Abstractions](https://en.wikipedia.org/wiki/Zero-overhead_principle)
