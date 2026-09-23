#!/usr/bin/env python3
import json
import sys
from pathlib import Path
from statistics import mean, stdev, median


def load_benchmarks(filename: str) -> list[dict]:
    with open(filename, "r") as f:
        data = json.load(f)
    return data.get("benchmarks", [])


def parse_benchmark_name(name: str) -> tuple[str, str]:
    parts = name.split("_", 2)
    if len(parts) < 3:
        return "Unknown", name

    approach = parts[1]
    operation = parts[2].split("/", 1)[0]
    return approach, operation


def group_benchmarks(benchmarks: list[dict]) -> dict[str, dict[str, list[dict]]]:
    grouped = {}
    for bench in benchmarks:
        if bench.get("run_type") == "aggregate":
            continue

        approach, op = parse_benchmark_name(bench["name"])

        grouped.setdefault(op, {}).setdefault(approach, []).append(bench)

    return grouped


def calculate_metrics(benches: list[dict]) -> dict[str, float]:
    times = [b["real_time"] for b in benches]

    return {
        "min": min(times),
        "max": max(times),
        "mean": mean(times),
        "median": median(times),
        "stdev": stdev(times) if len(times) > 1 else 0,
        "samples": len(times),
    }


def compare_approaches(
    template_benches: list[dict], erased_benches: list[dict]
) -> dict[str, dict | float]:
    t_metrics = calculate_metrics(template_benches)
    e_metrics = calculate_metrics(erased_benches)

    t_mean = t_metrics["mean"]
    e_mean = e_metrics["mean"]

    ratio = e_mean / t_mean if t_mean > 0 else 0
    improvement_pct = ((e_mean - t_mean) / e_mean * 100) if e_mean > 0 else 0

    return {
        "template": t_metrics,
        "erased": e_metrics,
        "ratio": ratio,
        "improvement_pct": improvement_pct,
    }


def print_comparison_table(grouped: dict[str, dict[str, list[dict]]]):
    print("\n" + "=" * 100)
    print("BENCHMARK COMPARISON: Template vs Type-Erased")
    print("=" * 100)

    print(
        f"\n{'Operation':<20} {'Template (ns)':<15} {'Erased (ns)':<15} {'Ratio':<10} {'Improvement':<15}"
    )
    print("-" * 100)

    for op in sorted(grouped.keys()):
        approaches = grouped[op]

        if "Template" in approaches and "Erased" in approaches:
            comparison = compare_approaches(
                approaches["Template"], approaches["Erased"]
            )

            t_mean = comparison["template"]["mean"]
            e_mean = comparison["erased"]["mean"]
            ratio = comparison["ratio"]
            improvement = comparison["improvement_pct"]

            print(
                f"{op:<20} {t_mean:>13.2f} {e_mean:>15.2f} {ratio:>8.2f}x {improvement:>12.1f}%"
            )


def print_detailed_stats(grouped: dict[str, dict[str, list[dict]]]):
    """Print detailed statistics for each operation"""
    print("\n" + "=" * 100)
    print("DETAILED STATISTICS BY OPERATION")
    print("=" * 100)

    for op in sorted(grouped.keys()):
        print(f"\n### {op}")
        print("-" * 100)

        approaches = grouped[op]

        for approach in sorted(approaches.keys()):
            benches = approaches[approach]
            metrics = calculate_metrics(benches)

            print(f"\n  {approach}:")
            print(f"    Mean:     {metrics['mean']:>10.2f} ns")
            print(f"    Median:   {metrics['median']:>10.2f} ns")
            print(f"    Min:      {metrics['min']:>10.2f} ns")
            print(f"    Max:      {metrics['max']:>10.2f} ns")
            print(f"    StdDev:   {metrics['stdev']:>10.2f} ns")
            print(f"    Samples:  {metrics['samples']:>10}")


def print_summary(grouped: dict[str, dict[str, list[dict]]]):
    """Print summary statistics"""
    print("\n" + "=" * 100)
    print("SUMMARY STATISTICS")
    print("=" * 100)

    all_template_times = []
    all_erased_times = []

    for op in grouped.keys():
        approaches = grouped[op]
        if "Template" in approaches:
            all_template_times.extend([b["real_time"] for b in approaches["Template"]])
        if "Erased" in approaches:
            all_erased_times.extend([b["real_time"] for b in approaches["Erased"]])

    if all_template_times:
        template_mean = mean(all_template_times)
        print(f"\nTemplate-based Average: {template_mean:.2f} ns")

    if all_erased_times:
        erased_mean = mean(all_erased_times)
        print(f"Type-erased Average:   {erased_mean:.2f} ns")

    if all_template_times and all_erased_times:
        overall_ratio = erased_mean / template_mean
        overall_improvement = (erased_mean - template_mean) / erased_mean * 100
        print(f"\nOverall Speedup:       {overall_ratio:.2f}x faster (template-based)")
        print(f"Overall Improvement:   {overall_improvement:.1f}%")


def print_insights(grouped: dict[str, dict[str, list[dict]]]):
    print("\n" + "=" * 100)
    print("INSIGHTS & OBSERVATIONS")
    print("=" * 100)

    ratios = []
    for op in grouped.keys():
        approaches = grouped[op]
        if "Template" in approaches and "Erased" in approaches:
            comparison = compare_approaches(
                approaches["Template"], approaches["Erased"]
            )
            ratios.append((op, comparison["ratio"]))

    if ratios:
        ratios.sort(key=lambda x: x[1], reverse=True)

        print("\nOperations with largest speedup (Template vs Erased):")
        for op, ratio in ratios[:3]:
            print(f"  - {op:<20} {ratio:>6.2f}x slower (type-erased)")

        print("\nOperations with smallest speedup:")
        for op, ratio in ratios[-3:]:
            print(f"  - {op:<20} {ratio:>6.2f}x slower (type-erased)")

        print("\nInterpretation:")
        print("  - Larger ratios indicate more significant type-erasure overhead")
        print("  - Simple operations show larger gaps (more inlining benefit)")
        print("  - Complex operations show smaller gaps (work dominates)")


def main():
    if len(sys.argv) < 2:
        print(
            "Usage: python scripts/analyze_benchmarks.py <JSON_results_file> [--detailed] [--insights]"
        )
        print("\nExample:")
        print("  python scripts/analyze_benchmarks.py results.json")
        print(
            "  python scripts/analyze_benchmarks.py results.json --detailed --insights"
        )
        sys.exit(1)

    json_file = sys.argv[1]
    show_detailed = "--detailed" in sys.argv
    show_insights = "--insights" in sys.argv

    if not Path(json_file).exists():
        print(f"Error: File '{json_file}' not found", file=sys.stderr)
        sys.exit(1)

    try:
        benchmarks = load_benchmarks(json_file)
        grouped = group_benchmarks(benchmarks)

        print_comparison_table(grouped)

        if show_detailed:
            print_detailed_stats(grouped)

        print_summary(grouped)

        if show_insights:
            print_insights(grouped)

        print("\n" + "=" * 100 + "\n")

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
