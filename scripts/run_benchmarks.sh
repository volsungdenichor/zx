#!/bin/bash
# Quick start script for running sequence benchmarks

set -e

# Resolve repository root from this script's location.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
BENCHMARK_EXE="$BUILD_DIR/modules/sequence/sequence_benchmarks"

echo "=== Sequence Benchmarks Quick Start ==="
echo

# Check if executable exists
if [ ! -f "$BENCHMARK_EXE" ]; then
    echo "Benchmark executable not found. Building..."
    cd "$PROJECT_DIR"
    cmake -B build \
        -DZX_BUILD_SEQUENCE=ON \
        -DZX_BUILD_BENCHMARKS=ON \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sequence_benchmarks -j$(nproc)
fi

echo "✓ Benchmark executable ready: $BENCHMARK_EXE"
echo

# Function to run benchmarks with different options
run_benchmark() {
    local name=$1
    local filter=$2
    local output=${3:-}

    echo "Running: $name"
    if [ -z "$output" ]; then
        "$BENCHMARK_EXE" --benchmark_filter="$filter"
    else
        "$BENCHMARK_EXE" --benchmark_filter="$filter" --benchmark_out="$output" --benchmark_out_format=json
        echo "Results saved to: $output"
    fi
    echo
}

# Display menu
echo "Select benchmark to run:"
echo "1. All benchmarks"
echo "2. Template-based only"
echo "3. Type-erased only"
echo "4. Specific operation (Transform, Filter, etc.)"
echo "5. Run and save to file"
echo "6. Export comparison (separate files)"
echo "0. Exit"
echo

read -p "Choice [0-6]: " choice

case $choice in
    1)
        echo "Running all benchmarks..."
        "$BENCHMARK_EXE"
        ;;
    2)
        run_benchmark "Template-based benchmarks" "BM_Template"
        ;;
    3)
        run_benchmark "Type-erased benchmarks" "BM_Erased"
        ;;
    4)
        read -p "Enter operation name (e.g., Transform, Filter, RangeIteration): " op
        run_benchmark "Operation: $op" "$op"
        ;;
    5)
        read -p "Enter output filename (default: benchmark_results.json): " filename
        filename=${filename:-benchmark_results.json}
        echo "Running benchmarks and saving to $filename..."
        "$BENCHMARK_EXE" --benchmark_out="$filename" --benchmark_out_format=json
        echo "✓ Results saved to: $filename"
        ;;
    6)
        echo "Exporting separate results for comparison..."
        echo "  - Saving template benchmarks to: template_results.json"
        "$BENCHMARK_EXE" --benchmark_filter="BM_Template" \
            --benchmark_out="template_results.json" \
            --benchmark_out_format=json
        echo "  - Saving erased benchmarks to: erased_results.json"
        "$BENCHMARK_EXE" --benchmark_filter="BM_Erased" \
            --benchmark_out="erased_results.json" \
            --benchmark_out_format=json
        echo "✓ Comparison files ready. You can now compare them:"
        echo "  python -m json.tool template_results.json"
        echo "  python -m json.tool erased_results.json"
        ;;
    0)
        echo "Exiting."
        exit 0
        ;;
    *)
        echo "Invalid choice."
        exit 1
        ;;
esac

echo
echo "=== Benchmark Complete ==="
echo
echo "For more detailed instructions, see:"
echo "  - $PROJECT_DIR/modules/sequence/benchmarks/README.md"
echo "  - $PROJECT_DIR/docs/BENCHMARKS_SUMMARY.md"
echo
echo "Useful benchmark options:"
echo "  --benchmark_filter=<regex>       Filter benchmarks by regex"
echo "  --benchmark_min_time=<seconds>   Minimum run time per benchmark"
echo "  --benchmark_repetitions=<n>      Number of repetitions"
echo "  --benchmark_out=<filename>       Output file name"
echo "  --benchmark_out_format=<format>  json, csv, or console"
echo
