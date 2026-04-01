# zx

A modular C++ utility library providing foundational data types, math primitives, lazy sequences, N-dimensional arrays, terminal rendering, computational geometry, and a data serialization format.

All modules are header-only (INTERFACE targets) and can be used individually or together.

---

## Modules

| Module | Description |
|---|---|
| [core](modules/core/README.md) | Result/optional types, Unicode strings, formatting, type traits |
| [iterator](modules/iterator/README.md) | Policy-based iterator construction, strided iterator |
| [functional](modules/functional/README.md) | Composable pipelines, `function_ref`, `let`, `do_all` |
| [sequence](modules/sequence/README.md) | Lazy pull-based sequences with rich adapters |
| [mat](modules/mat/README.md) | Vectors, matrices, and geometric primitives |
| [arrays](modules/arrays/README.md) | N-dimensional strided arrays with Python-like slicing |
| [ansi](modules/ansi/README.md) | ANSI terminal colors, font attributes, and surface renderer |
| [images](modules/images/README.md) | BMP image read/write |
| [geometry](modules/geometry/README.md) | DCEL mesh, Delaunay triangulation, Voronoi diagram |
| [nested_text](modules/nested_text/README.md) | S-expression-based data format — parser, printer, serialization |

---

## Requirements

- C++17 or newer
- CMake 3.14+

---

## Building

Clone and build all modules with tests:

```bash
git clone <url> zx
cd zx
cmake --preset ninja-debug
cmake --build build/ninja-debug
ctest --test-dir build/ninja-debug
```

Available presets (defined in `CMakePresets.json`):

| Preset | Build type | Tests | Benchmarks |
|---|---|---|---|
| `ninja-debug` | Debug | ON | OFF |
| `ninja-release` | Release | ON | OFF |

Build only specific modules:

```bash
cmake -B build \
  -DZXBUILD_CORE=ON \
  -DZXBUILD_SEQUENCE=ON \
  -DZX_BUILD_TESTS=ON
cmake --build build
```

`ZX_BUILD_ALL=ON` enables all modules in a single flag.

---

## Using as a dependency (FetchContent)

```cmake
include(FetchContent)

FetchContent_Declare(
    zx
    GIT_REPOSITORY git@github.com:volsungdenichor/zx.git
    GIT_TAG        main
)

set(ZX_BUILD_CORE     ON  CACHE BOOL "" FORCE)
set(ZX_BUILD_SEQUENCE ON  CACHE BOOL "" FORCE)
set(ZX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zx)

target_link_libraries(my_target PRIVATE zx::sequence)
```

Each module is available as `zx::<module_name>`. The aggregate `zx::zx` target links all enabled modules.
