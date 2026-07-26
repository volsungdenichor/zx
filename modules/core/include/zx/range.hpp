#pragma once

#include <zx/iterator_range.hpp>
#include <zx/iterators.hpp>

namespace zx
{

namespace detail
{

struct range_fn
{
    template <class T>
    auto operator()(T lower, T upper) const -> iterator_range_t<numeric_iterator<T>>
    {
        return iterator_range_t<numeric_iterator<T>>{ numeric_iterator<T>{ lower }, numeric_iterator<T>{ upper } };
    }

    template <class T>
    auto operator()(T upper) const -> iterator_range_t<numeric_iterator<T>>
    {
        return (*this)(T{ 0 }, upper);
    }
};

}  // namespace detail

static constexpr inline auto range = detail::range_fn{};

}  // namespace zx
