# zx/yield

Push-based stream processing for `zx`. The module is built around three pieces:

- generators produce values
- transducers transform or filter values in flight
- reductors consume values into a final state

Pipelines compose with `|=` and execute immediately when a generator is connected to a reductor.

---

## Header and CMake target

```cpp
#include <zx/yield.hpp>
```

Link against:

```cmake
target_link_libraries(my_target PRIVATE zx::yield)
```

---

## Basic pipeline

```cpp
std::vector<int> values =
    zx::range(1, 10)
    |= zx::filter([](int x) { return x % 2 == 0; })
    |= zx::transform([](int x) { return x * x; })
    |= zx::into(std::vector<int>{});

// values == {4, 16, 36, 64}
```

The left side is a generator, the middle stages are transducers, and the right side is a reductor returning the final state.

---

## Generators

Generators push values downstream until the pipeline finishes or a downstream stage requests `zx::step_t::loop_break`.

| API | Description |
| --- | --- |
| `zx::range(upper)` | Yields `0, 1, ..., upper - 1` |
| `zx::range(lower, upper)` | Yields `lower, ..., upper - 1` |
| `zx::iota(start)` | Yields an unbounded increasing sequence starting at `start` |
| `zx::from(range)` | Yields elements from an existing range |
| `zx::generate(fn)` | Builds a custom generator from a callable |

### `generate`

Custom generators receive a `yield` callable. Returning `zx::step_t::loop_break` stops generation early.

```cpp
auto triples =
    zx::generate(
        [](auto&& yield)
        {
            for (int a = 1; a <= 20; ++a)
            {
                for (int b = a; b <= 20; ++b)
                {
                    for (int c = b; c <= 20; ++c)
                    {
                        if (a * a + b * b == c * c)
                        {
                            if (yield(a, b, c) == zx::step_t::loop_break)
                            {
                                return;
                            }
                        }
                    }
                }
            }
        })
    |= zx::take(3)
    |= zx::transform([](int a, int b, int c) {
           return std::array<int, 3>{ a, b, c };
       })
    |= zx::into(std::vector<std::array<int, 3>>{});
```

---

## Transducers

Transducers are composable adapters that sit between a generator and a reductor.

| API | Description |
| --- | --- |
| `zx::transform(fn)` | Maps yielded values |
| `zx::transform_indexed(fn)` | Maps values with an additional zero-based index |
| `zx::filter(pred)` | Keeps only matching values |
| `zx::filter_indexed(pred)` | Indexed filter |
| `zx::take_while(pred)` | Stops forwarding when the predicate first fails |
| `zx::take_while_indexed(pred)` | Indexed `take_while` |
| `zx::drop_while(pred)` | Skips initial values while the predicate is true |
| `zx::drop_while_indexed(pred)` | Indexed `drop_while` |
| `zx::take(count)` | Forwards at most `count` values |
| `zx::drop(count)` | Skips the first `count` values |
| `zx::join` | Flattens a stream of ranges |
| `zx::intersperse(separator)` | Inserts a separator between forwarded values |

### Indexed stages

Indexed variants receive the zero-based position before the value.

```cpp
auto out =
    zx::range(1, 5)
    |= zx::transform_indexed([](std::size_t index, int x) {
           return x * static_cast<int>(index + 1);
       })
    |= zx::into(std::vector<int>{});

// out == {1, 4, 9, 16}
```

### Flattening and separators

```cpp
std::string joined =
    zx::from(std::vector<std::string>{ "Abc", "De", "Fghi" })
    |= zx::intersperse(std::string_view{ ", " })
    |= zx::join
    |= zx::into(std::string{});

// joined == "Abc, De, Fghi"
```

---

## Reductors

Reductors own the final state of the pipeline and decide whether processing continues.

| API | Result |
| --- | --- |
| `zx::copy_to(out_it)` | Writes to an output iterator |
| `zx::into(container)` | Appends into a container via `push_back` |
| `zx::all_of(pred)` | `bool` |
| `zx::any_of(pred)` | `bool` |
| `zx::none_of(pred)` | `bool` |
| `zx::fork(r0, r1, ...)` | `std::tuple<...>` of all reducer states |
| `zx::sum(init)` | Running sum starting from `init` |
| `zx::count()` | `std::size_t` element count |
| `zx::partition(pred, on_true, on_false)` | `std::tuple<true_state, false_state>` |
| `zx::accumulate(init, fn)` | General fold |
| `zx::for_each(fn)` | Executes side effects for each value |
| `zx::for_each_indexed(fn)` | Indexed side effects |
| `zx::out(reductor)` | Output iterator facade over a reductor |

### `fork`

Use `fork` when one pass should produce several results.

```cpp
auto [items, count, sum] =
    zx::range(1, 10)
    |= zx::fork(
           zx::into(std::vector<int>{}),
           zx::count(),
           zx::sum(0));

// items == {1, 2, 3, 4, 5, 6, 7, 8, 9}
// count == 9
// sum == 45
```

### `partition`

```cpp
auto [evens, odds] =
    zx::range(1, 10)
    |= zx::partition(
           [](int x) { return x % 2 == 0; },
           zx::transform([](int x) { return x * 10; }) |= zx::into(std::vector<int>{}),
           zx::into(std::vector<int>{}));

// evens == {20, 40, 60, 80}
// odds == {1, 3, 5, 7, 9}
```

### `out`

`zx::out(...)` adapts a reductor into an output iterator so the pipeline can be fed by standard algorithms.

```cpp
std::vector<int> out;
std::vector<int> in{ 2, 3, 5, 7 };

std::copy(
    in.begin(),
    in.end(),
    zx::out(zx::transform([](int x) { return x * 2; }) |= zx::copy_to(std::back_inserter(out))));

// out == {4, 6, 10, 14}
```

---

## Composition helpers

`zx::combine(...)` groups several transducers into one transducer.

```cpp
auto even_squares = zx::combine(
    zx::filter([](int x) { return x % 2 == 0; }),
    zx::transform([](int x) { return x * x; }));

auto values = zx::range(1, 10) |= even_squares |= zx::into(std::vector<int>{});
```

`zx::transduce(...)` applies transducers to a reductor first and returns a new reductor.

```cpp
auto collect_even_squares = zx::transduce(
    zx::filter([](int x) { return x % 2 == 0; }),
    zx::transform([](int x) { return x * x; }),
    zx::into(std::vector<int>{}));

auto values = zx::range(1, 10) |= collect_even_squares;
```

---

## Notes

- `zx::into(container)` expects `push_back` on the container state.
- `zx::copy_to(out_it)` is the better fit for output iterators and pre-sized destinations.
- `zx::yield` is push-based. If you want a pull-based lazy view style API, use `zx::sequence` instead.