#pragma once

#include <algorithm>
#include <optional>
#include <utility>
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
struct iterator_range_base_t
{
    using iterator = Iter;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr iterator_range_base_t() = default;
    constexpr iterator_range_base_t(const iterator_range_base_t&) = default;
    constexpr iterator_range_base_t(iterator_range_base_t&&) noexcept = default;

    constexpr iterator_range_base_t(iterator b, iterator e) : m_iterators(b, e) { }

    constexpr iterator_range_base_t(const std::pair<iterator, iterator>& pair) : m_iterators(pair) { }

    constexpr iterator_range_base_t(iterator b, difference_type n) : iterator_range_base_t(b, std::next(b, n)) { }

    template <class Range, class It = iterator_t<Range>, std::enable_if_t<std::is_constructible_v<iterator, It>, int> = 0>
    constexpr iterator_range_base_t(Range&& range) : iterator_range_base_t(std::begin(range), std::end(range))
    {
    }

    constexpr auto begin() const noexcept -> iterator { return m_iterators.first; }

    constexpr auto end() const noexcept -> iterator { return m_iterators.second; }

    void swap(iterator_range_base_t& other) noexcept { std::swap(m_iterators, other.m_iterators); }

    std::pair<iterator, iterator> m_iterators;
};

template <class T>
struct iterator_range_base_t<T*>
{
    using iterator = T*;
    using pointer = iterator;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr iterator_range_base_t() = default;
    constexpr iterator_range_base_t(const iterator_range_base_t&) = default;
    constexpr iterator_range_base_t(iterator_range_base_t&&) noexcept = default;

    constexpr iterator_range_base_t(iterator b, iterator e) : m_iterators(b, e) { }

    constexpr iterator_range_base_t(const std::pair<iterator, iterator>& pair) : m_iterators(pair) { }

    template <class D = difference_type, std::enable_if_t<std::is_integral_v<D>, int> = 0>
    constexpr iterator_range_base_t(iterator b, D n)
        : iterator_range_base_t(b, std::next(b, static_cast<difference_type>(n)))
    {
    }

    template <class Range, class It = iterator_t<Range>, std::enable_if_t<std::is_constructible_v<iterator, It>, int> = 0>
    constexpr iterator_range_base_t(Range&& range) : iterator_range_base_t(std::begin(range), std::end(range))
    {
    }

    template <
        class Range,
        class It = iterator_t<Range>,
        class Ptr = decltype(std::declval<Range>().data()),
        std::enable_if_t<std::is_constructible_v<iterator, Ptr> && !std::is_constructible_v<iterator, It>, int> = 0>
    constexpr iterator_range_base_t(Range&& range)
        : iterator_range_base_t(range.data(), static_cast<difference_type>(range.size()))
    {
    }

    constexpr auto begin() const noexcept -> iterator { return m_iterators.first; }

    constexpr auto end() const noexcept -> iterator { return m_iterators.second; }

    constexpr auto data() const noexcept -> pointer { return begin(); }

    void swap(iterator_range_base_t& other) noexcept { std::swap(m_iterators, other.m_iterators); }

    std::pair<iterator, iterator> m_iterators;
};

}  // namespace detail

struct iterator_range_slice_t
{
    std::optional<std::ptrdiff_t> start;
    std::optional<std::ptrdiff_t> stop;
};

template <class Iter>
struct iterator_range_t : detail::iterator_range_base_t<Iter>
{
    using base_t = detail::iterator_range_base_t<Iter>;

    using iterator = typename base_t::iterator;
    using difference_type = typename base_t::difference_type;
    using size_type = difference_type;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;

    using value_type = typename std::iterator_traits<iterator>::value_type;

    using reverse_type = iterator_range_t<detail::reverse_iterator_t<iterator>>;

    using base_t::base_t;
    using base_t::begin;
    using base_t::end;

    iterator_range_t& operator=(iterator_range_t other) noexcept
    {
        base_t::swap(other);
        return *this;
    }

    template <class Container, std::enable_if_t<std::is_constructible_v<Container, iterator, iterator>, int> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto empty() const -> bool { return begin() == end(); }

    template <class It = iterator, std::enable_if_t<is_random_access_iterator<It>::value, int> = 0>
    auto ssize() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, std::enable_if_t<is_random_access_iterator<It>::value, int> = 0>
    auto size() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    auto front() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range("iterator_range_t::front - empty range");
        }

        return *begin();
    }

    template <class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value, int> = 0>
    auto back() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range("iterator_range_t::back - empty range");
        }
        return *std::prev(end());
    }

    template <class It = iterator, std::enable_if_t<is_random_access_iterator<It>::value, int> = 0>
    auto at(difference_type n) const -> reference
    {
        if (!(0 <= n && n < size()))
        {
            throw std::out_of_range("iterator_range_t::at - index out of range");
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, std::enable_if_t<is_random_access_iterator<It>::value, int> = 0>
    auto operator[](difference_type n) const -> reference
    {
        return at(n);
    }

    template <class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value, int> = 0>
    auto reverse() const -> reverse_type
    {
        return detail::make_reverse(begin(), end());
    }

    auto take(difference_type n) const -> iterator_range_t { return iterator_range_t(begin(), advance(n)); }

    auto drop(difference_type n) const -> iterator_range_t { return iterator_range_t(advance(n), end()); }

    template <class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value, int> = 0>
    auto take_back(difference_type n) const -> iterator_range_t
    {
        return reverse().take(n).reverse();
    }

    template <class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value, int> = 0>
    auto drop_back(difference_type n) const -> iterator_range_t
    {
        return reverse().drop(n).reverse();
    }

    template <class Pred>
    auto take_while(Pred&& pred) const -> iterator_range_t
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range_t(begin(), found);
    }

    template <class Pred>
    auto drop_while(Pred&& pred) const -> iterator_range_t
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range_t(found, end());
    }

    template <class Pred, class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value, int> = 0>
    auto take_back_while(Pred&& pred) const -> iterator_range_t
    {
        return reverse().take_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred, class It = iterator, std::enable_if_t<is_bidirectional_iterator<It>::value> = 0>
    auto drop_back_while(Pred&& pred) const -> iterator_range_t
    {
        return reverse().drop_while(std::forward<Pred>(pred)).reverse();
    }

    template <class It = iterator, std::enable_if_t<is_random_access_iterator<It>::value, int> = 0>
    auto slice(const iterator_range_slice_t& info) const -> iterator_range_t
    {
        static const auto adjust = [](difference_type index,
                                      size_type size) -> size_type {  //
            return std::clamp<size_type>(index >= 0 ? index : index + size, 0, size);
        };
        const size_type s = size();
        const size_type b = info.start ? adjust(*info.start, s) : size_type{ 0 };
        const size_type e = info.stop ? adjust(*info.stop, s) : s;
        return iterator_range_t{ begin() + b, std::max(size_type{ 0 }, e - b) };
    }

private:
    auto advance(difference_type n) const -> iterator
    {
        if constexpr (is_random_access_iterator<iterator>::value)
        {
            return begin() + std::min(ssize(), n);
        }
        else
        {
            iterator it = begin();
            while (it != end() && n > 0)
            {
                --n;
                ++it;
            }
            return it;
        }
    }
};

template <class T>
using span_t = iterator_range_t<const T*>;

template <class T>
using mut_span_t = iterator_range_t<T*>;

template <class Container>
using subrange_t = iterator_range_t<iterator_t<Container>>;

}  // namespace zx
