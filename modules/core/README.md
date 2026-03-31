# zx/core

Foundational utilities used throughout the `zx` library. This module provides error handling types, optional wrappers, Unicode strings, range views, formatting, type-traits, and type name demangling.

---

## `result_t<T, E>` — `<zx/result.hpp>`

A discriminated union holding either a value of type `T` or an error of type `E`. Three specializations are provided: `result_t<T, E>` (value by copy), `result_t<T&, E>` (value by reference), and `result_t<void, E>` (success with no value).

Errors are constructed via the `zx::error()` helper to distinguish them from values at the call site.

### Construction

```cpp
result_t<T, E> ok_result{ value };           // holds a value
result_t<T, E> err_result{ zx::error(e) };   // holds an error
result_t<void, E> ok_void{};                 // success (no value)
result_t<void, E> err_void{ zx::error(e) };  // error
```

### State inspection

| Expression      | Return type | Description              |
| --------------- | ----------- | ------------------------ |
| `(bool) r`      | `bool`      | `true` if holds a value  |
| `r.has_value()` | `bool`      | `true` if holds a value  |
| `r.has_error()` | `bool`      | `true` if holds an error |

### Value / error access

| Expression             | Return type | Description                                 |
| ---------------------- | ----------- | ------------------------------------------- |
| `*r`                   | `T&`        | Value — throws `bad_result_access` if error |
| `r.value()`            | `T&`        | Same as `*r`                                |
| `r->member`            | `T*`        | Pointer to value                            |
| `r.error()`            | `E&`        | Error — throws `bad_result_access` if value |
| `r.value_or(fallback)` | `T`         | Value, or `fallback` if error               |

### Monadic operations

#### `transform` — map value, preserve error

```cpp
result_t<T, E> r = ...;
result_t<U, E> s = r.transform([](T v) -> U { ... });
// value path: s holds func(v)
// error path: s holds the original error
```

For `result_t<void, E>`, the function takes no arguments:
```cpp
result_t<U, E> s = r.transform([]() -> U { ... });
```

#### `and_then` — flatMap over value (function returns a result)

```cpp
result_t<T, E> r = ...;
result_t<U, E> s = r.and_then([](T v) -> result_t<U, E> { ... });
// value path: s = func(v)
// error path: s holds the original error
```

#### `transform_error` — map error, preserve value

```cpp
result_t<T, E> r = ...;
result_t<T, F> s = r.transform_error([](E e) -> F { ... });
// value path: s holds the original value
// error path: s holds zx::error(func(e))
```

#### `or_else` — recover from error

**With a function returning `result_t`** — can change the error type:
```cpp
result_t<T, E> r = ...;
result_t<T, F> s = r.or_else([](E e) -> result_t<T, F> { ... });
// value path: s holds the original value
// error path: s = func(e)
```

**With a `void`-returning function** — side-effect only, type unchanged:
```cpp
result_t<T, E> s = r.or_else([](E e) { /* log, etc. */ });
// value path: unchanged
// error path: func(e) called, original result returned
```

### Equality

Two `result_t` values are equal when both hold values that compare equal, or both hold errors that compare equal. A value and an error are never equal.

```cpp
result_t<T, E> a = ..., b = ...;
bool eq = (a == b);
bool ne = (a != b);
```

### Chaining example

```cpp
result_t<T, E> r = parse(input);

auto final = r
    .transform([](T v)          -> U   { return process(v); })
    .and_then ([](U v)          -> result_t<V, E> { return validate(v); })
    .transform_error([](E e)    -> F   { return remap(e); })
    .value_or(V{});
```

### Exception type

`bad_result_access` (inherits `std::runtime_error`) is thrown when accessing a value on an error result or vice versa.

---

## `maybe_t<T>` — `<zx/maybe.hpp>`

An optional wrapper holding either a value of type `T` or nothing. Two specializations are provided: `maybe_t<T>` (value by copy) and `maybe_t<T&>` (value by reference). The sentinel value `zx::none` represents the empty state.

### Construction

```cpp
maybe_t<T> full{ value };   // holds a value
maybe_t<T> empty{};         // holds nothing
maybe_t<T> empty{ none };   // holds nothing (explicit)
```

Converting construction from `maybe_t<U>` is supported when `U` is convertible to `T`.

### State inspection

| Expression      | Return type | Description             |
| --------------- | ----------- | ----------------------- |
| `(bool) m`      | `bool`      | `true` if holds a value |
| `m.has_value()` | `bool`      | `true` if holds a value |

### Value access

| Expression        | Return type | Description                                       |
| ----------------- | ----------- | ------------------------------------------------- |
| `*m`              | `T&`        | Value — throws `bad_maybe_access` if empty        |
| `m.value()`       | `T&`        | Same as `*m`                                      |
| `m->member`       | `T*`        | Pointer to value                                  |
| `m.value_or(v)`   | `T`         | Value, or `v` if empty                            |

### Monadic operations

#### `transform` — map value, propagate empty

```cpp
maybe_t<T> m = ...;
maybe_t<U> n = m.transform([](T v) -> U { ... });
// value path: n holds func(v)
// empty path: n is empty
```

#### `and_then` — flatMap over value (function returns a maybe)

```cpp
maybe_t<T> m = ...;
maybe_t<U> n = m.and_then([](T v) -> maybe_t<U> { ... });
// value path: n = func(v)
// empty path: n is empty
```

#### `or_else` — provide a fallback when empty

**With a function returning `maybe_t`** — can change the value type:
```cpp
maybe_t<T> n = m.or_else([]() -> maybe_t<T> { return fallback(); });
// value path: unchanged
// empty path: n = func()
```

**With a `void`-returning function** — side-effect only, type unchanged:
```cpp
maybe_t<T> n = m.or_else([]() { /* log, etc. */ });
// value path: unchanged
// empty path: func() called, original (empty) maybe returned
```

#### `filter` — conditionally clear the value

```cpp
maybe_t<T> n = m.filter([](const T& v) -> bool { ... });
// value path: n holds v if pred(v) is true, otherwise empty
// empty path: n is empty
```

### Equality

| Expression        | Meaning                                       |
| ----------------- | --------------------------------------------- |
| `m == n`          | Both empty, or both hold values that compare equal |
| `m != n`          | Negation of the above                         |
| `m == value`      | `m` holds a value equal to `value`            |
| `m != value`      | Negation of the above                         |
| `m == none`       | `m` is empty                                  |
| `m != none`       | `m` holds a value                             |

### Chaining example

```cpp
maybe_t<T> m = lookup(key);

auto result = m
    .filter ([](const T& v)  -> bool      { return is_valid(v); })
    .transform([](T v)       -> U         { return process(v); })
    .and_then ([](U v)       -> maybe_t<V>{ return resolve(v); })
    .value_or(V{});
```

### Exception type

`bad_maybe_access` (inherits `std::exception`) is thrown when accessing the value of an empty `maybe_t`.
