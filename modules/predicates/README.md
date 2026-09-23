# zx/predicates

**CMake target:** `predicates`
**Dependencies:** `core`, `nested_text`

The `predicates` module provides reusable, composable predicates for filtering and validating values.

## Header

```cpp
#include <zx/predicates.hpp>
```

## Core idea

Most predicate factories return a callable object with:

- `pred.test(value)` -> `bool`
- `pred(value)` -> `bool` (same as `test`)
- `pred.validate(value)` -> `validation_result_t`

`validation_result_t` can represent either success or a rich `validation_error_t` with:

- `expected`: predicate description
- `actual`: encoded input value
- `reason`: structured failure details

## Primitive predicates

- `eq(x)`: value equals `x`
- `ne(x)`: value not equal to `x`
- `gt(x)`: value greater than `x`
- `lt(x)`: value less than `x`
- `ge(x)`: value greater than or equal to `x`
- `le(x)`: value less than or equal to `x`
- `divisible_by(d)`: integer value divisible by `d`

## Composition predicates

- `all(p1, p2, ...)`: all predicates must match
- `any(p1, p2, ...)`: at least one predicate must match
- `is_not(p)`: logical negation

## Range predicates

- `items_all(p)`: every item in a range matches `p`
- `items_any(p)`: at least one item in a range matches `p`
- `items_none(p)`: no item in a range matches `p`
- `items_count(p)`: predicate over `range` length
- `items_are(p1, p2, ...)`: positional matching against an exact number of items

## Projection predicates

- `result_of(name, fn, p)`: applies `fn(value)` and validates the result with `p`
- `field(...)` and `property(...)`: aliases of `result_of(...)`

## Example

```cpp
#include <vector>
#include <zx/predicates.hpp>

using namespace zx::predicates;

const auto code_pred = all(ge(1000), lt(1100), divisible_by(7));

const bool ok = code_pred(1001); // true
const auto bad = code_pred.validate(1000); // structured failure (not divisible_by 7)

const auto vec_pred = items_are(5, ge(10), divisible_by(3));
const bool vec_ok = vec_pred(std::vector{5, 11, 21}); // true
```

## Linking

```cmake
target_link_libraries(my_target PRIVATE zx::predicates)
```
