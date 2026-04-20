load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

ZX_TEST_COPTS = [
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wshadow",
    "-Wnon-virtual-dtor",
    "-Wold-style-cast",
    "-Wcast-align",
    "-Woverloaded-virtual",
    "-Wunused",
    # Treat googletest/googlemock headers as system headers to suppress
    # warnings from their internals (mirrors CMake's GTest::gmock IMPORTED target
    # which uses -isystem automatically).
    "-isystem", "external/googletest+/googletest/include",
    "-isystem", "external/googletest+/googlemock/include",
]

def zx_module(name, deps = []):
    cc_library(
        name = name,
        hdrs = native.glob(["include/**/*.hpp"]),
        includes = ["include"],
        deps = deps,
        visibility = ["//visibility:public"],
    )

def zx_module_test(name, module, srcs, data = [], defines = [], extra_copts = []):
    cc_test(
        name = name,
        srcs = srcs,
        copts = ZX_TEST_COPTS + extra_copts,
        data = data,
        defines = defines,
        deps = [
            module,
            "@googletest//:gtest_main",
        ],
    )
