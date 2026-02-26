#pragma once

#include <zx/iterator_interface.hpp>

namespace zx
{

namespace detail
{
template <class Iter, std::ptrdiff_t N>
struct strided_iterator_impl
{
    static_assert(is_random_access_iterator<Iter>::value, "strided_iterator: random access iterator required");

    Iter m_inner;

    strided_iterator_impl() = default;
    strided_iterator_impl(Iter inner) : m_inner{ inner } { }

    constexpr iter_reference_t<Iter> deref() const { return *m_inner; }

    constexpr void inc() { m_inner += N; }

    constexpr void dec() { m_inner -= N; }

    constexpr void advance(std::ptrdiff_t n) { m_inner += n * N; }

    constexpr bool is_equal(const strided_iterator_impl& other) const { return m_inner == other.m_inner; }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        return N > 0 ? m_inner < other.m_inner : m_inner > other.m_inner;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const { return (other.m_inner - m_inner) / N; }
};

template <class Iter>
struct strided_iterator_impl<Iter, 0>
{
    static_assert(is_random_access_iterator<Iter>::value, "strided_iterator: random access iterator required");

    Iter m_inner;
    std::ptrdiff_t m_stride;

    strided_iterator_impl() = default;
    strided_iterator_impl(Iter inner, std::ptrdiff_t stride = 0) : m_inner{ inner }, m_stride{ stride } { }

    constexpr iter_reference_t<Iter> deref() const { return *m_inner; }

    constexpr void inc() { m_inner += m_stride; }

    constexpr void dec() { m_inner -= m_stride; }

    constexpr void advance(std::ptrdiff_t n) { m_inner += n * m_stride; }

    constexpr bool is_equal(const strided_iterator_impl& other) const { return m_inner == other.m_inner; }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return m_stride > 0 ? m_inner < other.m_inner : m_inner > other.m_inner;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return (other.m_inner - m_inner) / m_stride;
    }
};

}  // namespace detail

template <class Iter, std::ptrdiff_t N = 0>
using strided_iterator = zx::iterator_interface<detail::strided_iterator_impl<Iter, N>>;

}  // namespace zx