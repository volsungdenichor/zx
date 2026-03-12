# Sequence Benchmarks - Summary

## Overview

Successfully created comprehensive Google Benchmark-based performance benchmarks for the `sequence_t` template-based implementation in the ZX library. The benchmarks compare the performance of:

1. **Type-Erased (std::function-based) Approach**
   - Uses `std::function<iteration_result_t<T>()>` for the next function
   - Provides type erasure with runtime overhead
   - Patterns: `BM_Erased_*`

2. **Template-Based (Non-Type-Erased) Approach**
   - Keeps `NextFn` as a template parameter in `sequence_t<T, NextFn>`
   - Enables zero-cost abstractions and full inlining
   - Patterns: `BM_Template_*`

## Files Created/Modified

### New Files
- **[modules/sequence/benchmarks/sequence.bench.cpp](../modules/sequence/benchmarks/sequence.bench.cpp)**
  - Contains 14 benchmark functions (7 operation pairs)
  - Comprehensive test coverage for common sequence operations
  - Properly handles benchmark state and optimization barriers

- **[modules/sequence/benchmarks/README.md](../modules/sequence/benchmarks/README.md)**
  - Complete documentation for running and interpreting benchmarks
  - Detailed descriptions of each benchmark operation
  - Tips for consistent measurements and result analysis

### Modified Files
- **[CMakeLists.txt (root)](../CMakeLists.txt)**
  - Added `ZX_BUILD_BENCHMARKS` option
  - Added Google Benchmark FetchContent configuration
  - Integrated benchmark build infrastructure

- **[cmake/ZxHelpers.cmake](../cmake/ZxHelpers.cmake)**
  - Added `BENCHMARK_SOURCES` parameter to `zx_add_module()` function
  - Integrated benchmark executable creation and linking

- **[modules/sequence/CMakeLists.txt](../modules/sequence/CMakeLists.txt)**
  - Added benchmark sources to module configuration

- **[modules/sequence/include/zx/sequence.hpp](../modules/sequence/include/zx/sequence.hpp)**
  - Added missing `#include <memory>` header for `std::shared_ptr` support

## Benchmark Operations

### 1. RangeIteration
- **Test**: Create range 0..N, iterate and sum elements
- **Purpose**: Baseline iteration overhead
- **Complexity**: O(N)

### 2. Transform
- **Test**: Range + single transformation (multiply by 2)
- **Purpose**: Measure function call and inlining overhead
- **Complexity**: O(N) with extra work

### 3. Filter
- **Test**: Range + filter for even numbers
- **Purpose**: Measure predicate evaluation and branching
- **Complexity**: O(N) with conditional execution

### 4. Chained
- **Test**: Range → filter (even) → transform (×2) → transform (+1)
- **Purpose**: Measure composition overhead of multiple operations
- **Complexity**: O(N) with composition layers

### 5. DropTake
- **Test**: Range → drop(10) → take(10)
- **Purpose**: Measure stateful operations (skip, limit)
- **Complexity**: O(N) with state bookkeeping

### 6. Step
- **Test**: Range → step(2) - every 2nd element
- **Purpose**: Measure strided iteration
- **Complexity**: O(N) with modulo arithmetic

### 7. Fold (for_each)
- **Test**: Range → for_each with accumulation
- **Purpose**: Measure final reduction operation
- **Complexity**: O(N) with side effects

## Building the Benchmarks

### Option 1: From Scratch
```bash
# From the repository root
rm -rf build
cmake -B build \
  -DZX_BUILD_SEQUENCE=ON \
  -DZX_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target sequence_benchmarks -j$(nproc)
```

### Option 2: Using Existing Build
```bash
cd build
cmake .. -DZX_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
make sequence_benchmarks -j4
```

## Running the Benchmarks

### Basic Execution
```bash
./build/modules/sequence/sequence_benchmarks
```

### Filtered Execution
```bash
# Run only template-based benchmarks
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Template

# Run only type-erased benchmarks
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Erased

# Run only a specific operation
./build/modules/sequence/sequence_benchmarks --benchmark_filter=Transform
```

### Export Results
```bash
# JSON output for analysis
./build/modules/sequence/sequence_benchmarks \
  --benchmark_out=results.json \
  --benchmark_out_format=json

# CSV output
./build/modules/sequence/sequence_benchmarks \
  --benchmark_out=results.csv \
  --benchmark_out_format=csv
```

## Expected Performance Characteristics

### Template-Based Advantages
- **Inlining**: Direct function call inlining without virtual indirection
- **Specialization**: Type-specific optimizations per sequence operation
- **Zero-Cost Abstraction**: Minimal to no runtime overhead
- **Branch Prediction**: Compiler has complete visibility for optimization

### Type-Erased Costs
- **Indirection**: Virtual function calls through `std::function`
- **Type Information Loss**: Unable to specialize on concrete types
- **Small Object Optimization Limits**: Fixed size buffer may not accommodate all use cases
- **Runtime Dispatch**: Function pointer dereferencing

### Expected Performance Gap
| Operation | Gap Factor | Notes |
|-----------|-----------|-------|
| Simple Iteration | 2-3x slower | Heavy type erasure cost |
| Transform | 2-5x slower | Lambda call overhead |
| Filter | 2-4x slower | Predicate evaluation |
| Chained | 1.5-2x slower | Composition dominates |
| Drop/Take | 1-2x slower | Less overhead |
| Step | 1.5-2x slower | Modulo overhead |
| Fold | Similar | Memory operation dominates |

## Implementation Details

### Benchmark Framework
- **Library**: Google Benchmark v1.8.3
- **Integration**: CMake FetchContent
- **Build Type**: Release (-O3) for accurate performance measurements
- **Compiler Flags**: Strict warnings enabled for code quality

### Type Definitions
```cpp
// Type-erased implementation for comparison
template <class T>
using erased_next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
using erased_sequence_t = sequence_t<T, erased_next_function_t<T>>;
```

### Optimization Barriers
- `benchmark::DoNotOptimize()`: Prevents compiler from eliminating computations
- Range parameters: 100 to 100,000 elements for scalability testing
- Multiple iterations: Automatic calibration for statistical significance

## Files Location

```
zx/
├── docs/
│   ├── BENCHMARKS_INDEX.md                     (new)
│   └── BENCHMARKS_SUMMARY.md                   (this file)
├── CMakeLists.txt                              (modified)
├── cmake/ZxHelpers.cmake                       (modified)
└── modules/sequence/
    ├── CMakeLists.txt                          (modified)
    ├── include/zx/sequence.hpp                 (modified - added #include <memory>)
    └── benchmarks/
        ├── sequence.bench.cpp                  (new - 350+ lines)
        └── README.md                           (new - comprehensive guide)
```

## Next Steps

### Running the Benchmarks
1. Build the project with `cmake --build build --target sequence_benchmarks`
2. Execute `./build/modules/sequence/sequence_benchmarks`
3. Analyze output for performance characteristics

### Further Optimization Opportunities
1. **Small Function Optimization**: Store small lambdas inline
2. **Lazy Evaluation**: Defer composition until iteration starts
3. **Vectorization**: SIMD support for element-wise operations
4. **Move Semantics**: Better move support in chained operations
5. **Inline Caching**: Cache frequently-used operations

### Benchmark Extensions
- Add memory allocation benchmarks
- Benchmark with different capture sizes
- Test with more complex data types
- Measure threading scalability
- Profile cache efficiency

## Compilation Notes

### Successfully Compiled With
- C++17 standard
- Release optimization (-O3)
- Strict compiler warnings enabled via `zx_set_strict_warnings()`
- Both Clang and GCC compatibility

### Dependencies
- Google Benchmark v1.8.3 (automatic via FetchContent)
- ZX sequence module
- Standard C++ library with `<memory>` support

## Performance Analysis Tools

### Recommended Tools
- `perf` - Linux performance profiler
- `flamegraph` - Visualization of hot code paths
- `callgrind` - Detailed call profiling
- `google-benchmark` CLI tools - Direct benchmark analysis

### Analysis Commands
```bash
# Profile benchmarks with perf
perf record ./build/modules/sequence/sequence_benchmarks

# Generate flame graph
perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > out.svg

# Compare two benchmark runs
tools/compare.py results1.json results2.json
```

## Validation

The benchmarks have been:
- ✅ Successfully compiled without errors
- ✅ Linked against Google Benchmark library
- ✅ Configured with proper CMake integration
- ✅ Integrated into the zx project build system
- ✅ Ready for execution and performance analysis