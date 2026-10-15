#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <utility>
#include <zx/maybe.hpp>
#include <zx/pipe.hpp>
#include <zx/type_traits.hpp>

namespace zx

{

namespace detail
{

template <class Iter>
struct reverse_iterator_impl
{
    using type = std::reverse_iterator<Iter>;
};

template <class Iter>
struct reverse_iterator_impl<std::reverse_iterator<Iter>>
{
    using type = Iter;
};

template <class Iter>
using reverse_iterator_t = typename reverse_iterator_impl<Iter>::type;

template <class Iter>
auto make_reverse(Iter b, Iter e) -> std::pair<reverse_iterator_t<Iter>, reverse_iterator_t<Iter>>
{
    return { reverse_iterator_t<Iter>(e), reverse_iterator_t<Iter>(b) };
}

template <class Iter>
auto make_reverse(std::reverse_iterator<Iter> b, std::reverse_iterator<Iter> e) -> std::pair<Iter, Iter>
{
    return { e.base(), b.base() };
}

template <class Iter>
auto advance(Iter begin, Iter end, std::ptrdiff_t n) -> Iter
{
    if constexpr (is_random_access_iterator<Iter>::value)
    {
        return begin + std::min(std::distance(begin, end), n);
    }
    else
    {
        Iter it = begin;
        while (it != end && n > 0)
        {
            --n;
            ++it;
        }
        return it;
    }
}

template <class Iter>
struct subrange_t
{
    using iterator = Iter;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr subrange_t() = default;
    constexpr subrange_t(const subrange_t&) = default;
    constexpr subrange_t(subrange_t&&) noexcept = default;

    constexpr subrange_t(iterator b, iterator e) : m_iterators(b, e) { }

    constexpr subrange_t(const std::pair<iterator, iterator>& pair) : m_iterators(pair) { }

    constexpr subrange_t(iterator b, difference_type n) : subrange_t(b, std::next(b, n)) { }

    template <class Range, class It = iterator_t<Range>, enable_if_t<std::is_constructible_v<iterator, It>> = 0>
    constexpr subrange_t(Range&& range) : subrange_t(std::begin(range), std::end(range))
    {
    }

    constexpr auto begin() const noexcept -> iterator { return m_iterators.first; }

    constexpr auto end() const noexcept -> iterator { return m_iterators.second; }

    void swap(subrange_t& other) noexcept { std::swap(m_iterators, other.m_iterators); }

    std::pair<iterator, iterator> m_iterators;
};

template <class T>
struct subrange_t<T*>
{
    using iterator = T*;
    using pointer = iterator;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr subrange_t() = default;
    constexpr subrange_t(const subrange_t&) = default;
    constexpr subrange_t(subrange_t&&) noexcept = default;

    constexpr subrange_t(iterator b, iterator e) : m_iterators(b, e) { }

    constexpr subrange_t(const std::pair<iterator, iterator>& pair) : m_iterators(pair) { }

    template <class D = difference_type, enable_if_t<std::is_integral_v<D>> = 0>
    constexpr subrange_t(iterator b, D n) : subrange_t(b, std::next(b, static_cast<difference_type>(n)))
    {
    }

    template <class Range, class It = iterator_t<Range>, enable_if_t<std::is_constructible_v<iterator, It>> = 0>
    constexpr subrange_t(Range&& range) : subrange_t(std::begin(range), std::end(range))
    {
    }

    template <
        class Range,
        class It = iterator_t<Range>,
        class Ptr = decltype(std::declval<Range>().data()),
        enable_if_t<std::is_constructible_v<iterator, Ptr>, !std::is_constructible_v<iterator, It>> = 0>
    constexpr subrange_t(Range&& range) : subrange_t(range.data(), static_cast<difference_type>(range.size()))
    {
    }

    constexpr auto begin() const noexcept -> iterator { return m_iterators.first; }

    constexpr auto end() const noexcept -> iterator { return m_iterators.second; }

    constexpr auto data() const noexcept -> pointer { return begin(); }

    void swap(subrange_t& other) noexcept { std::swap(m_iterators, other.m_iterators); }

    std::pair<iterator, iterator> m_iterators;
};

}  // namespace detail

struct subrange_slice_t
{
    std::optional<std::ptrdiff_t> start;
    std::optional<std::ptrdiff_t> stop;
};

template <class Iter>
struct subrange_t;

template <class Iter>
subrange_t(Iter, Iter) -> subrange_t<Iter>;

template <class Range>
subrange_t(Range&&) -> subrange_t<iterator_t<Range>>;

template <class Iter>
struct subrange_t : detail::subrange_t<Iter>
{
    using base_t = detail::subrange_t<Iter>;

    using iterator = typename base_t::iterator;
    using difference_type = typename base_t::difference_type;
    using size_type = difference_type;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;
    using maybe_reference = maybe_t<reference>;

    using value_type = typename std::iterator_traits<iterator>::value_type;

    using reverse_type = subrange_t<detail::reverse_iterator_t<iterator>>;
    using reverse_iterator = typename reverse_type::iterator;

    using base_t::base_t;
    using base_t::begin;
    using base_t::end;

    template <class It = iterator, enable_if_t<is_bidirectional_iterator<It>::value> = 0>
    auto rbegin() const -> reverse_iterator
    {
        return reverse_iterator{ end() };
    }

    template <class It = iterator, enable_if_t<is_bidirectional_iterator<It>::value> = 0>
    auto rend() const -> reverse_iterator
    {
        return reverse_iterator{ begin() };
    }

    template <class Container, enable_if_t<std::is_constructible_v<Container, iterator, iterator>> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto empty() const -> bool { return begin() == end(); }

    template <class It = iterator, enable_if_t<is_random_access_iterator<It>::value> = 0>
    auto ssize() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, enable_if_t<is_random_access_iterator<It>::value> = 0>
    auto size() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, enable_if_t<is_random_access_iterator<It>::value> = 0>
    auto at(difference_type n) const -> reference
    {
        if (0 <= n && n < ssize())
        {
            return begin()[n];
        }
        throw std::out_of_range("subrange_t::at - index out of range");
    }

    template <class It = iterator, enable_if_t<is_random_access_iterator<It>::value> = 0>
    auto operator[](difference_type n) const -> reference
    {
        return at(n);
    }

    template <class It = iterator, enable_if_t<is_bidirectional_iterator<It>::value> = 0>
    auto _reverse() const -> reverse_type
    {
        return detail::make_reverse(begin(), end());
    }

    auto _take(difference_type n) const -> subrange_t { return subrange_t(begin(), detail::advance(begin(), end(), n)); }

    template <class It = iterator, enable_if_t<is_bidirectional_iterator<It>::value> = 0>
    auto _take_back(difference_type n) const -> subrange_t
    {
        return _reverse()._take(n)._reverse();
    }

    template <class Range, class Compare = std::equal_to<>>
    bool is_equal(Range&& other, Compare&& compare = {}) const
    {
        auto other_range = subrange_t<iterator_t<Range>>{ std::forward<Range>(other) };
        return std::equal(begin(), end(), other_range.begin(), other_range.end(), std::forward<Compare>(compare));
    }

    template <class Range, class Compare = std::less<>>
    bool is_less(Range&& other, Compare&& compare = {}) const
    {
        auto other_range = subrange_t<iterator_t<Range>>{ std::forward<Range>(other) };
        return std::lexicographical_compare(
            begin(), end(), other_range.begin(), other_range.end(), std::forward<Compare>(compare));
    }

    template <class Range, class Compare = std::less<>>
    bool is_greater(Range&& other, Compare&& compare = {}) const
    {
        return subrange_t<iterator_t<Range>>{ std::forward<Range>(other) }.is_less(*this, std::forward<Compare>(compare));
    }

    template <class Range, class Compare = std::equal_to<>>
    bool starts_with(Range&& prefix, Compare&& compare = {}) const
    {
        auto prefix_range = subrange_t<iterator_t<Range>>{ std::forward<Range>(prefix) };
        return _take(prefix_range.size()).is_equal(prefix_range, std::forward<Compare>(compare));
    }

    template <class Range, class Compare = std::equal_to<>>
    bool ends_with(Range&& suffix, Compare&& compare = {}) const
    {
        auto suffix_range = subrange_t<iterator_t<Range>>{ std::forward<Range>(suffix) };
        return _take_back(suffix_range.size()).is_equal(suffix_range, std::forward<Compare>(compare));
    }

    template <class Range, class Compare = std::equal_to<>>
    bool contains_subrange(Range&& subrange, Compare&& compare = {}) const
    {
        auto subrange_range = subrange_t<iterator_t<Range>>{ std::forward<Range>(subrange) };
        return std::search(begin(), end(), subrange_range.begin(), subrange_range.end(), std::forward<Compare>(compare))
               != end();
    }
};

template <class L, class R>
bool operator==(subrange_t<L> lhs, R&& rhs)
{
    return lhs.is_equal(rhs);
}

template <class L, class R>
bool operator!=(subrange_t<L> lhs, R&& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R>
bool operator==(L&& lhs, subrange_t<R> rhs)
{
    return rhs == lhs;
}

template <class L, class R>
bool operator!=(L&& lhs, subrange_t<R> rhs)
{
    return !(lhs == rhs);
}

template <class L, class R>
bool operator<(subrange_t<L> lhs, R&& rhs)
{
    return lhs.is_less(rhs);
}

template <class L, class R>
bool operator>(subrange_t<L> lhs, R&& rhs)
{
    return lhs.is_greater(rhs);
}

template <class L, class R>
bool operator<=(subrange_t<L> lhs, R&& rhs)
{
    return !(lhs > rhs);
}

template <class L, class R>
bool operator>=(subrange_t<L> lhs, R&& rhs)
{
    return !(lhs < rhs);
}

template <class L, class R>
bool operator<(L&& lhs, subrange_t<R> rhs)
{
    return rhs > lhs;
}

template <class L, class R>
bool operator>(L&& lhs, subrange_t<R> rhs)
{
    return rhs < lhs;
}

template <class L, class R>
bool operator<=(L&& lhs, subrange_t<R> rhs)
{
    return !(lhs > rhs);
}

template <class L, class R>
bool operator>=(L&& lhs, subrange_t<R> rhs)
{
    return !(lhs < rhs);
}

template <class T>
using span_t = subrange_t<const T*>;

template <class T>
using mut_span_t = subrange_t<T*>;

namespace subrange
{

namespace detail
{

struct front_fn
{
    template <class Iter>
    auto operator()(subrange_t<Iter> value) const -> typename subrange_t<Iter>::reference
    {
        if (!value.empty())
        {
            return *value.begin();
        }
        throw std::out_of_range("subrange_t::front - empty range");
    }
};

struct back_fn
{
    template <class Iter, enable_if_t<is_bidirectional_iterator<Iter>::value> = 0>
    auto operator()(subrange_t<Iter> value) const -> typename subrange_t<Iter>::reference
    {
        if (!value.empty())
        {
            return *std::prev(value.end());
        }
        throw std::out_of_range("subrange_t::back - empty range");
    }
};

struct maybe_front_fn
{
    template <class Iter, class Res = typename subrange_t<Iter>::maybe_reference>
    auto operator()(subrange_t<Iter> value) const -> Res
    {
        return !value.empty() ? Res{ *value.begin() } : Res{};
    }
};

struct maybe_back_fn
{
    template <
        class Iter,
        class Res = typename subrange_t<Iter>::maybe_reference,
        enable_if_t<is_bidirectional_iterator<Iter>::value> = 0>
    auto operator()(subrange_t<Iter> value) const -> Res
    {
        return !value.empty() ? Res{ *std::prev(value.end()) } : Res{};
    }
};

struct maybe_at_fn
{
    template <class Iter, class Res = typename subrange_t<Iter>::maybe_reference>
    constexpr auto operator()(subrange_t<Iter> value, std::ptrdiff_t index) const -> Res
    {
        return (0 <= index && index < value.size()) ? Res{ *std::next(value.begin(), index) } : Res{};
    }

    auto operator()(std::ptrdiff_t index) const { return fn(std::bind(*this, std::placeholders::_1, index)); }
};

struct at_fn
{
    template <class Iter, class Res = typename subrange_t<Iter>::maybe_reference>
    constexpr auto operator()(subrange_t<Iter> value, std::ptrdiff_t index) const -> Res
    {
        if (0 <= index && index < value.ssize())
        {
            return Res{ *std::next(value.begin(), index) };
        }
        throw std::out_of_range("subrange_t::at - index out of range");
    }

    auto operator()(std::ptrdiff_t index) const { return fn(std::bind(*this, std::placeholders::_1, index)); }
};

struct reverse_fn
{
    template <class Iter, enable_if_t<is_bidirectional_iterator<Iter>::value> = 0>
    auto operator()(subrange_t<Iter> value) const -> typename subrange_t<Iter>::reverse_type
    {
        return ::zx::detail::make_reverse(value.begin(), value.end());
    }
};

template <bool First, bool Reverse>
struct take_drop_fn
{
    template <class Iter>
    auto operator()(subrange_t<Iter> value, std::ptrdiff_t n) const -> subrange_t<Iter>
    {
        if constexpr (Reverse)
        {
            static const auto reverse = reverse_fn{};
            static const auto self = take_drop_fn<First, false>{};
            return reverse(self(reverse(value), n));
        }
        else
        {
            const auto middle = ::zx::detail::advance(value.begin(), value.end(), n);
            if constexpr (First)
            {
                return subrange_t<Iter>{ value.begin(), middle };
            }
            else
            {
                return subrange_t<Iter>{ middle, value.end() };
            }
        }
    }

    auto operator()(std::ptrdiff_t n) const { return fn(std::bind(*this, std::placeholders::_1, n)); }
};

template <bool First, bool Reverse>
struct take_drop_while_fn
{
    template <class Iter, class Pred>
    auto operator()(subrange_t<Iter> value, const Pred& pred) const -> subrange_t<Iter>
    {
        if constexpr (Reverse)
        {
            static const auto reverse = reverse_fn{};
            static const auto self = take_drop_while_fn<First, false>{};
            return reverse(self(reverse(value), pred));
        }
        else
        {
            const auto middle = std::find_if_not(value.begin(), value.end(), pred);
            if constexpr (First)
            {
                return subrange_t<Iter>{ value.begin(), middle };
            }
            else
            {
                return subrange_t<Iter>{ middle, value.end() };
            }
        }
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct slice_fn
{
    template <class Iter, enable_if_t<is_random_access_iterator<Iter>::value> = 0>
    auto operator()(subrange_t<Iter> value, const subrange_slice_t& info) const -> subrange_t<Iter>
    {
        using size_type = typename subrange_t<Iter>::size_type;
        const size_type s = value.size();
        const size_type b = info.start ? adjust(*info.start, s) : size_type{ 0 };
        const size_type e = info.stop ? adjust(*info.stop, s) : s;
        return subrange_t<Iter>{ value.begin() + b, std::max(size_type{ 0 }, e - b) };
    }

    template <class Size>
    static auto adjust(Size index, Size size) -> Size
    {
        return std::clamp<Size>(index >= 0 ? index : index + size, 0, size);
    }

    auto operator()(subrange_slice_t info) const { return fn(std::bind(*this, std::placeholders::_1, info)); }
};

struct find_fn
{
    template <class Iter, class T, class Res = typename subrange_t<Iter>::maybe_reference>
    auto operator()(subrange_t<Iter> value, const T& element) const -> Res
    {
        const auto it = std::find(value.begin(), value.end(), element);
        return it != value.end() ? Res{ *it } : Res{};
    }

    template <class T>
    auto operator()(T&& element) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<T>(element)));
    }
};

struct find_if_fn
{
    template <class Iter, class Pred, class Res = typename subrange_t<Iter>::maybe_reference>
    auto operator()(subrange_t<Iter> value, const Pred& pred) const -> Res
    {
        const auto it = std::find_if(value.begin(), value.end(), pred);
        return it != value.end() ? Res{ *it } : Res{};
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct all_of_fn
{
    template <class Iter, class Pred>
    bool operator()(subrange_t<Iter> value, Pred&& pred) const
    {
        return std::all_of(value.begin(), value.end(), std::forward<Pred>(pred));
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct any_of_fn
{
    template <class Iter, class Pred>
    bool operator()(subrange_t<Iter> value, Pred&& pred) const
    {
        return std::any_of(value.begin(), value.end(), std::forward<Pred>(pred));
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct none_of_fn
{
    template <class Iter, class Pred>
    bool operator()(subrange_t<Iter> value, Pred&& pred) const
    {
        return std::none_of(value.begin(), value.end(), std::forward<Pred>(pred));
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

}  // namespace detail

constexpr inline auto front = fn(detail::front_fn{});
constexpr inline auto back = fn(detail::back_fn{});
constexpr inline auto maybe_front = fn(detail::maybe_front_fn{});
constexpr inline auto maybe_back = fn(detail::maybe_back_fn{});
constexpr inline auto maybe_at = detail::maybe_at_fn{};
constexpr inline auto at = detail::at_fn{};
constexpr inline auto reverse = fn(detail::reverse_fn{});
constexpr inline auto take = detail::take_drop_fn<true, false>{};
constexpr inline auto drop = detail::take_drop_fn<false, false>{};
constexpr inline auto take_back = detail::take_drop_fn<true, true>{};
constexpr inline auto drop_back = detail::take_drop_fn<false, true>{};
constexpr inline auto take_while = detail::take_drop_while_fn<true, false>{};
constexpr inline auto drop_while = detail::take_drop_while_fn<false, false>{};
constexpr inline auto take_back_while = detail::take_drop_while_fn<true, true>{};
constexpr inline auto drop_back_while = detail::take_drop_while_fn<false, true>{};
constexpr inline auto slice = detail::slice_fn{};
constexpr inline auto find = detail::find_fn{};
constexpr inline auto find_if = detail::find_if_fn{};

constexpr inline auto all_of = detail::all_of_fn{};
constexpr inline auto any_of = detail::any_of_fn{};
constexpr inline auto none_of = detail::none_of_fn{};

}  // namespace subrange

}  // namespace zx
