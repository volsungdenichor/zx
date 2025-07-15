# zx

A bunch of **C++17** functionality which is always needed for creating expressive code, but lacks in C++ standard or appears so lazily that it's better to write it on one's own than to wait until it appears in C++29/32/35...

## result
  - transform

    > result<🍏, 🍅>.**transform**(_func_: (🍏) -> 🍌) -> result<🍌, 🍅>

  - transform_error

    > result<🍏, 🍅>.**transform_error**(_func_: (🍅) -> 🍒) -> result<🍏, 🍒>

  - and_then

    > result<🍏, 🍅>.**and_then**(_func_: (🍏) -> result<🍌, 🍅>) -> result<🍌, 🍅>

  - or_else

    > result<🍏, 🍅>.**or_else**(_func_: (🍅) -> result<🍏, 🍅>) -> result<🍏, 🍅>

    > result<🍏, 🍅>.**or_else**(_func_: (🍅) -> void) -> result<🍏, 🍅>

  - value_or

    > result<🍏, 🍅>.**value_or**(_fallback_value_: 🍏) -> 🍏

  - value

    > result<🍏, 🍅>.**value**() -> 🍏

  - error

    > result<🍏, 🍅>.**error**() -> 🍅

  - has_value

    > result<🍏, 🍅>.**has_value**() -> bool

  - has_error

    > result<🍏, 🍅>.**has_error**() -> bool

## maybe
  - transform

    > maybe<🍏>.**transform**(_func_: (🍏) -> 🍊) -> maybe<🍊>

  - and_then

    > maybe<🍏>.**and_then**(_func_: (🍏) -> maybe<🍊>) -> maybe<🍊>

  - filter

    > maybe<🍏>.**filter**(_pred_: (🍏) -> bool) -> maybe<🍏>

  - or_else

    > maybe<🍏>.**or_else**(_func_: () -> maybe<🍏>) -> maybe<🍏>

    > maybe<🍏>.**or_else**(_func_: () -> void) -> maybe<🍏>

  - value_or

    > maybe<🍏>.**value_or**(_fallback_value_: 🍏) -> 🍏

  - value

    > maybe<🍏>.**value**() -> 🍏

  - has_value

    > maybe<🍏>.**has_value**() -> bool

## sequence
  - maybe_front
  - maybe_at
  - find_if
  - index_of
  - copy
  - accumulate
  - inspect
  - inspect_indexed
  - transform
  - transform_indexed
  - filter
  - filter_indexed
  - transform_maybe
  - transform_maybe_indexed
  - drop_while
  - drop_while_indexed
  - take_while
  - take_while_indexed
  - drop
  - take
  - step
  - join
  - for_each
  - for_each_indexed
  - intersperse
  - seq::iota
  - seq::range
  - seq::unfold
  - seq::view
  - seq::owning
  - seq::repeat
  - seq::single
  - seq::concat
  - seq::vec
  - seq::zip
  - seq::init
  - seq::init_infinite
  - seq::get_lines

## iterator_range
  - empty
  - size
  - ssize
  - at
  - operator[]
  - maybe_at
  - front
  - maybe_front
  - back
  - maybe_back
  - take
  - drop
  - take_while
  - drop_while
  - take_back
  - drop_back
  - take_back_while
  - drop_back_while
  - slice
  - find_of

## functional
  - pipe
  - do_all
  - apply
  - with

## type traits

## integer types
  - i8, u8, i16, u16, i32, u32, i64, u64
  - f32, f64
  - char32
