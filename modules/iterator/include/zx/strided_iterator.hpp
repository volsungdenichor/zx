#pragma once

#include <zx/iterator_interface.hpp>

namespace zx
{

namespace detail
{
template <class T, std::ptrdiff_t N>
struct strided_iterator_impl
{
    T* m_ptr;

    explicit strided_iterator_impl(T* ptr = {}) : m_ptr{ ptr } { }

    constexpr T& deref() const { return *m_ptr; }

    constexpr void inc() { m_ptr += N; }

    constexpr void dec() { m_ptr -= N; }

    constexpr void advance(std::ptrdiff_t n) { m_ptr += n * N; }

    constexpr bool is_equal(const strided_iterator_impl& other) const { return m_ptr == other.m_ptr; }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        return N > 0 ? m_ptr < other.m_ptr : m_ptr > other.m_ptr;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const { return (other.m_ptr - m_ptr) / N; }
};

template <class T>
struct strided_iterator_impl<T, 0>
{
    T* m_ptr;

    std::ptrdiff_t m_stride;

    strided_iterator_impl(T* ptr = {}, std::ptrdiff_t stride = 0) : m_ptr{ ptr }, m_stride{ stride } { }

    constexpr T& deref() const { return *m_ptr; }

    constexpr void inc() { m_ptr += m_stride; }

    constexpr void dec() { m_ptr -= m_stride; }

    constexpr void advance(std::ptrdiff_t n) { m_ptr += n * m_stride; }

    constexpr bool is_equal(const strided_iterator_impl& other) const { return m_ptr == other.m_ptr; }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return m_stride > 0 ? m_ptr < other.m_ptr : m_ptr > other.m_ptr;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return (other.m_ptr - m_ptr) / m_stride;
    }
};

}  // namespace detail

template <class T, std::ptrdiff_t N = 0>
using strided_iterator = zx::iterator_interface<detail::strided_iterator_impl<T, N>>;

}  // namespace zx