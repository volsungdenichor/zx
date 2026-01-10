# ZX - Modern C++ Utilities Library

ZX is a modern C++ utilities library providing essential building blocks for functional programming, iterator abstractions, type traits, and formatting. The library is organized into modular components that can be used independently or together.

## Table of Contents

- [ZX - Modern C++ Utilities Library](#zx---modern-c-utilities-library)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Modules](#modules)
    - [Core Module](#core-module)
      - [Format](#format)
      - [Type Traits](#type-traits)
    - [Functional Module](#functional-module)
      - [Function Reference](#function-reference)
      - [Pipeline \& Pipe](#pipeline--pipe)
      - [Functional Utilities](#functional-utilities)
    - [Iterator Module](#iterator-module)
      - [Iterator Interface](#iterator-interface)
      - [Iterator Range](#iterator-range)
  - [Building](#building)
  - [Usage Examples](#usage-examples)
    - [Formatting Example](#formatting-example)
    - [Pipeline Example](#pipeline-example)
    - [Iterator Range Example](#iterator-range-example)

---

## Overview

ZX provides a collection of modern C++ utilities designed to simplify common programming patterns:

- **Type-safe formatting** with extensible formatter support
- **Functional programming** primitives including pipelines and function composition
- **Iterator abstractions** to simplify custom iterator creation
- **Type traits** utilities for template metaprogramming
- **Range utilities** for working with iterator pairs

The library follows modern C++ practices with extensive use of `constexpr`, SFINAE, and type traits.

---

## Modules

### Core Module

The core module provides fundamental utilities including formatting and type traits.

#### Format

**Location**: [modules/core/include/zx/format.hpp](modules/core/include/zx/format.hpp)

Type-safe formatting utilities with extensible formatter support for custom types.

**Key Components**:

- **`format(...)`** - Format arguments to a string
  ```cpp
  zx::format("Value: ", 42, ", Pi: ", 3.14)  // "Value: 42, Pi: 3.14"
  zx::format(std::pair{true, 42})             // "(true, 42)"
  zx::format(std::tuple{'x', true, 42})       // "(x, true, 42)"
  ```

- **`format_to(ostream, ...)`** - Format arguments to an output stream
  ```cpp
  zx::format_to(std::cout, "Hello", " ", "World");
  ```

- **`print(...)`** - Print to stdout without newline
  ```cpp
  zx::print("Count: ", 42);
  ```

- **`println(...)`** - Print to stdout with newline
  ```cpp
  zx::println("Hello, World!");
  ```

- **`delimit(range, separator)`** - Format range elements with a delimiter
  ```cpp
  zx::format(zx::delimit(std::vector<int>{1, 2, 3}, "-"))  // "1-2-3"
  ```

- **`formatter<T>`** - Extensible formatter template for custom types
  ```cpp
  template <>
  struct zx::formatter<MyType> {
      void format(std::ostream& os, const MyType& item) const {
          os << "MyType(" << item.value << ")";
      }
  };
  ```

**Built-in Formatters**:
- `bool` - formats as "true" or "false"
- `std::tuple<...>` - formats as "(elem1, elem2, ...)"
- `std::pair<F, S>` - formats as "(first, second)"
- Any type with `operator<<` defined

#### Type Traits

**Location**: [modules/core/include/zx/type_traits.hpp](modules/core/include/zx/type_traits.hpp)

Extended type traits and metaprogramming utilities.

**Key Components**:

- **`is_detected<Op, Args...>`** - Detects if an operation is valid
  ```cpp
  template <class T>
  using has_size = decltype(std::declval<T>().size());

  static_assert(zx::is_detected_v<has_size, std::vector<int>>);
  ```

- **`type_identity<T>`** - Identity metafunction (useful for preventing type deduction)

- **`always_false<T>`** - Always evaluates to false (for static_assert)

- **Iterator Type Aliases**:
  - `iterator_t<T>` - Iterator type of a range
  - `iter_category_t<T>` - Iterator category
  - `iter_reference_t<T>` - Iterator reference type
  - `iter_value_t<T>` - Iterator value type
  - `iter_difference_t<T>` - Iterator difference type

- **Range Type Aliases**:
  - `range_category_t<T>` - Iterator category of range
  - `range_reference_t<T>` - Reference type of range elements
  - `range_value_t<T>` - Value type of range elements
  - `range_difference_t<T>` - Difference type of range

- **Iterator Category Traits**:
  - `is_input_iterator<T>` / `is_input_range<T>`
  - `is_forward_iterator<T>` / `is_forward_range<T>`
  - `is_bidirectional_iterator<T>` / `is_bidirectional_range<T>`
  - `is_random_access_iterator<T>` / `is_random_access_range<T>`

---

### Functional Module

The functional module provides utilities for functional programming including pipelines, function composition, and function references.

#### Function Reference

**Location**: [modules/functional/include/zx/function_ref.hpp](modules/functional/include/zx/function_ref.hpp)

Lightweight, non-owning reference to callable objects (similar to `std::function` but without allocation).

**Key Features**:
- Non-owning reference to any callable
- No heap allocation
- Cannot be default-constructed
- Type-erased function wrapper

**Usage**:
```cpp
void process(zx::function_ref<int(int)> func) {
    return func(42);
}

auto lambda = [](int x) { return x * 2; };
process(lambda);  // Works with any callable
```

#### Pipeline & Pipe

**Location**: [modules/functional/include/zx/pipe.hpp](modules/functional/include/zx/pipe.hpp)

Function composition and pipelining utilities for creating data transformation pipelines.

**Key Components**:

- **`pipeline<Pipes...>`** - Composable function pipeline
  ```cpp
  auto pipe = zx::pipe(
      std::plus<>{},
      [](int x) { return std::to_string(x); },
      [](const std::string& s) { return "[" + s + "]"; }
  );
  pipe(3, 4);  // "[7]"
  ```

- **`pipe(funcs...)`** / **`fn(funcs...)`** - Create a pipeline
  ```cpp
  auto pipe = zx::fn(std::plus<>{})
      |= zx::fn([](int x) { return std::to_string(x); })
      |= zx::fn([](const std::string& s) { return "[" + s + "]"; });
  ```

- **`operator|=(item, pipeline)`** - Apply pipeline to a value
  ```cpp
  auto result = value |= pipeline;
  ```

- **`operator|=(pipeline1, pipeline2)`** - Compose pipelines
  ```cpp
  auto combined = pipe1 |= pipe2;
  ```

**Usage Examples**:
```cpp
// Create a pipeline
auto process = zx::pipe(
    [](int x) { return x * 2; },
    [](int x) { return x + 10; }
);

int result = process(5);  // (5 * 2) + 10 = 20

// Using operator|=
auto extended = process |= zx::fn([](int x) { return std::to_string(x); });
```

#### Functional Utilities

**Location**: [modules/functional/include/zx/functional.hpp](modules/functional/include/zx/functional.hpp)

Higher-order functions for side-effect operations and functional composition.

**Key Components**:

- **`apply(funcs...)`** - Apply functions and return reference to original (modifies in-place)
  ```cpp
  std::string str = "hello";
  str |= zx::apply(uppercase, take(4));
  // str is now "HELL"
  ```

- **`with(funcs...)`** - Apply functions to a copy and return the modified copy
  ```cpp
  const std::string str = "hello";
  auto result = str |= zx::with(uppercase, take(4));
  // str is still "hello", result is "HELL"
  ```

- **`do_all(funcs...)`** - Execute multiple functions with the same arguments
  ```cpp
  auto multi = zx::do_all(
      [](int x) { std::cout << "A: " << x; },
      [](int x) { std::cout << "B: " << x; }
  );
  multi(42);  // Prints both
  ```

---

### Iterator Module

The iterator module provides utilities for creating custom iterators and working with iterator ranges.

#### Iterator Interface

**Location**: [modules/iterator/include/zx/iterator_interface.hpp](modules/iterator/include/zx/iterator_interface.hpp)

CRTP-based iterator interface that simplifies custom iterator creation by implementing required operations based on minimal user-provided primitives.

**Required Methods**:
- `deref()` - Dereference the iterator (required)
- `is_equal(other)` OR `distance_to(other)` - Equality comparison (required)
- Default constructor (required)

**Optional Methods** (enable additional iterator categories):
- `inc()` OR `advance(n)` - Enable forward iteration
- `dec()` OR `advance(n)` - Enable bidirectional iteration
- `is_less(other)` OR `distance_to(other)` - Enable ordering
- `distance_to(other)` - Enable random access

**Provided Operations**:
- `operator*`, `operator->`
- `operator++`, `operator++(int)` (if incrementable)
- `operator--`, `operator--(int)` (if decrementable)
- `operator==`, `operator!=` (always available)
- `operator<`, `operator>`, `operator<=`, `operator>=` (if comparable)
- `operator+`, `operator-`, `operator+=`, `operator-=` (if random access)

**Usage Example**:
```cpp
struct my_iterator_impl {
    int* ptr;

    int& deref() const { return *ptr; }
    void inc() { ++ptr; }
    void dec() { --ptr; }
    bool is_equal(const my_iterator_impl& other) const {
        return ptr == other.ptr;
    }
};

using my_iterator = zx::iterator_interface<my_iterator_impl>;
```

#### Iterator Range

**Location**: [modules/iterator/include/zx/iterator_range.hpp](modules/iterator/include/zx/iterator_range.hpp)

Lightweight wrapper around iterator pairs providing range-based operations.

**Key Features**:

- **Construction**:
  ```cpp
  zx::iterator_range range1(begin, end);           // From iterator pair
  zx::iterator_range range2(begin, count);         // From start + count
  zx::iterator_range range3(container);            // From any container
  ```

- **Range Operations**:
  - `begin()`, `end()` - Iterator access
  - `empty()` - Check if range is empty
  - `size()`, `ssize()` - Get range size (random access only)
  - `front()`, `back()` - Access first/last element
  - `operator[]` - Index access (random access only)
  - `data()` - Pointer access (for pointer iterators)

- **Slicing and Subranges**:
  ```cpp
  auto sub = range.slice(2, 5);           // Elements from index 2 to 5
  auto first_n = range.take(10);          // First 10 elements
  auto skip_n = range.drop(5);            // Skip first 5 elements
  auto last_n = range.take_back(10);      // Last 10 elements
  auto drop_last = range.drop_back(5);    // Drop last 5 elements
  ```

- **Reverse Iteration**:
  ```cpp
  auto reversed = range.reverse();        // Reverse the range
  ```

- **Conversion**:
  ```cpp
  std::vector<int> vec = range;           // Convert to container
  ```

**Usage Example**:
```cpp
std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
zx::iterator_range range(vec);

auto middle = range.slice(2, 7);        // {3, 4, 5, 6, 7}
auto first_five = range.take(5);        // {1, 2, 3, 4, 5}
auto reversed = range.reverse();        // {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}

for (int x : middle) {
    std::cout << x << " ";
}
```

---

## Building

The library uses CMake as its build system. To build with GCC:

```bash
cd build
cmake ..
make
```

To use Clang instead of GCC:

```bash
cd build
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
make
```

Or using environment variables:

```bash
cd build
CC=clang CXX=clang++ cmake ..
make
```

To run tests:

```bash
cd build
ctest
```

---

## Usage Examples

### Formatting Example

```cpp
#include <zx/format.hpp>

int main() {
    // Basic formatting
    zx::println("Answer: ", 42);

    // Format collections
    std::vector<int> nums = {1, 2, 3, 4, 5};
    zx::println(zx::delimit(nums, ", "));

    // Custom types with formatter
    auto point = std::pair{10, 20};
    zx::println("Point: ", point);  // "Point: (10, 20)"
}
```

### Pipeline Example

```cpp
#include <zx/pipe.hpp>
#include <zx/functional.hpp>

int main() {
    // Create a data transformation pipeline
    auto process = zx::pipe(
        [](int x) { return x * 2; },
        [](int x) { return x + 10; },
        [](int x) { return std::to_string(x); }
    );

    auto result = process(5);  // "20"

    // Using operator|=
    std::string str = "hello";
    str |= zx::apply(
        [](std::string& s) { s[0] = std::toupper(s[0]); }
    );
    // str is now "Hello"
}
```

### Iterator Range Example

```cpp
#include <zx/iterator_range.hpp>

int main() {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    zx::iterator_range range(data);

    // Take a slice
    auto middle = range.slice(2, 7);

    // Process the slice
    for (int x : middle) {
        std::cout << x << " ";  // 3 4 5 6 7
    }
}
```