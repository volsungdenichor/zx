zx_add_module(
    NAME sequence
    INTERFACE

    TEST_SOURCES
    tests/sequence.test.cpp

    BENCHMARK_SOURCES
    benchmarks/sequence.bench.cpp
)