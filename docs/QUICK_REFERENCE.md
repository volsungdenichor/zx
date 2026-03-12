# Sequence Benchmarks - Quick Reference Card

## One-Liners

```bash
# Build everything
cmake -B build -DZX_BUILD_BENCHMARKS=ON -DZX_BUILD_SEQUENCE=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build --target sequence_benchmarks -j$(nproc)

# Run all benchmarks
./build/modules/sequence/sequence_benchmarks

# Run just template benchmarks
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Template

# Save results to JSON
./build/modules/sequence/sequence_benchmarks --benchmark_out=results.json --benchmark_out_format=json

# Analyze results
python scripts/analyze_benchmarks.py results.json --detailed --insights

# Compare two approaches (side by side)
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Template --benchmark_out=tmpl.json --benchmark_out_format=json && \
./build/modules/sequence/sequence_benchmarks --benchmark_filter=BM_Erased --benchmark_out=eras.json --benchmark_out_format=json && \
python scripts/analyze_benchmarks.py tmpl.json
```

## Key Benchmark Names

```
BM_Erased_RangeIteration            # Type-erased: iterate 0..N
BM_Template_RangeIteration          # Template-based: iterate 0..N

BM_Erased_Transform                 # Type-erased: transform(x*2)
BM_Template_Transform               # Template-based: transform(x*2)

BM_Erased_Filter                    # Type-erased: filter(even)
BM_Template_Filter                  # Template-based: filter(even)

BM_Erased_Chained                   # Type-erased: filter→transform→transform
BM_Template_Chained                 # Template-based: filter→transform→transform

BM_Erased_DropTake                  # Type-erased: drop(10)→take(10)
BM_Template_DropTake                # Template-based: drop(10)→take(10)

BM_Erased_Step                      # Type-erased: step(2)
BM_Template_Step                    # Template-based: step(2)

BM_Erased_Fold                      # Type-erased: for_each with sum
BM_Template_Fold                    # Template-based: for_each with sum
```

## Common Options

```bash
--benchmark_filter=<regex>           Match benchmarks by regex
--benchmark_out=<file>               Output file name
--benchmark_out_format=json|csv      Export format
--benchmark_min_time=<seconds>s      Minimum benchmark time
--benchmark_repetitions=<n>          Number of repetitions
--benchmark_time_unit=ns|us|ms|s     Output time unit
--benchmark_report_aggregates_only   Show only aggregates
--help                               Show all options
```

## Filter Examples

```bash
# Just one operation
--benchmark_filter=Transform

# By approach
--benchmark_filter=BM_Template
--benchmark_filter=BM_Erased

# Multiple patterns
--benchmark_filter=Transform|Filter

# Exclude pattern
--benchmark_filter=^(?!.*Chained)  # Exclude Chained
```

## Output Interpretation

```
Benchmark                   Time         CPU      Iterations
─────────────────────────────────────────────────────────────
BM_Template_RangeIteration  127 ns      127 ns     978371
  └─ 127 ns = time per iteration
  └─ 978371 = how many times ran

Ratio = Erased / Template = 301 / 127 ≈ 2.4x
```

## File Locations

```
Benchmarks
├─ executable: ./build/modules/sequence/sequence_benchmarks
├─ source: ./modules/sequence/benchmarks/sequence.bench.cpp
└─ docs: ./modules/sequence/benchmarks/README.md

Documentation
├─ Quick Start: ./docs/BENCHMARKS_INDEX.md
├─ Summary: ./docs/BENCHMARKS_SUMMARY.md
├─ Complete: ./docs/IMPLEMENTATION_COMPLETE.md
└─ Scripts: ./scripts/run_benchmarks.sh, ./scripts/analyze_benchmarks.py
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `sequence_benchmarks not found` | Build with `cmake --build build --target sequence_benchmarks` |
| `shared_ptr error` | Already fixed, rebuild with `-j4` |
| `Noisy results` | Close background apps, disable turbo: `echo 1 \| sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo` |
| `Missing --benchmark_min_time suffix` | Use: `--benchmark_min_time=1s` (not just `1`) |
| `Can't analyze results` | Make sure to use `--benchmark_out_format=json` |

## Performance Tips

1. **Disable background processes** for more consistent results
2. **Disable turbo boost** for thermal stability
3. **Use Release builds** (-O3 optimizations) only
4. **Run multiple times** to verify consistency
5. **Focus on larger ranges** (10k+) for more realistic costs

## Expected Speedups

```
Simple Operations (2-3x faster):
  ✓ RangeIteration, Transform, Filter

Moderate Operations (1.5-2x faster):
  ✓ Chained, Step

Stateful Operations (1-2x faster):
  ✓ DropTake, Fold
```

## One-Command Workflow

```bash
# Build, run, export, and analyze all in one go
(cd <repo-root> && \
 cmake -B build -DZX_BUILD_BENCHMARKS=ON -DZX_BUILD_SEQUENCE=ON -DCMAKE_BUILD_TYPE=Release && \
 cmake --build build --target sequence_benchmarks -j$(nproc) && \
 ./build/modules/sequence/sequence_benchmarks --benchmark_out=results.json --benchmark_out_format=json && \
 python scripts/analyze_benchmarks.py results.json --detailed --insights)
```

## CSV/JSON Export

```bash
# Save as CSV
./build/modules/sequence/sequence_benchmarks --benchmark_out=results.csv --benchmark_out_format=csv

# Save as JSON
./build/modules/sequence/sequence_benchmarks --benchmark_out=results.json --benchmark_out_format=json

# Both formats
for fmt in json csv; do
  ./build/modules/sequence/sequence_benchmarks \
    --benchmark_out=results.$fmt --benchmark_out_format=$fmt
done
```

## Interactive Menu

```bash
bash scripts/run_benchmarks.sh  # Opens menu for common operations
```

## Key Takeaways

✅ **Template-based is faster** - Zero-cost abstractions win
✅ **Gap varies by operation** - Simple ops show bigger gains
✅ **Easy to measure** - Benchmarks are straightforward to run
✅ **Well documented** - Complete guides and examples provided
✅ **Tools included** - Analysis scripts make comparison easy

## Next Steps

1. Run: `./build/modules/sequence/sequence_benchmarks`
2. Export: Add `--benchmark_out=results.json --benchmark_out_format=json`
3. Analyze: `python scripts/analyze_benchmarks.py results.json --insights`
4. Profile: Use `perf record` for hot spot analysis
5. Optimize: Focus on identified bottlenecks

---

**Quick Start**: `bash scripts/run_benchmarks.sh`
**Full Docs**: `cat docs/BENCHMARKS_INDEX.md`
**Analysis**: `python scripts/analyze_benchmarks.py --help`