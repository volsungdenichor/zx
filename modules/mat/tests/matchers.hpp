#pragma once

#include <gmock/gmock.h>

#include <zx/mat.hpp>

namespace detail
{

template <class, class = void>
struct ApproxEqualMatcher;

template <std::size_t R, std::size_t C, class T>
struct ApproxEqualMatcher<zx::mat::matrix_t<R, C, T>>
{
    using is_gtest_matcher = void;

    zx::mat::matrix_t<R, C, T> m_expected;

    ApproxEqualMatcher(zx::mat::matrix_t<R, C, T> expected) : m_expected{ expected } { }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::matrix_t<R, C, U>& actual, ::testing::MatchResultListener* listener) const
    {
        constexpr auto epsilon = std::common_type_t<T, U>(0.1);

        bool result = true;

        for (std::size_t r = 0; r < R; ++r)
        {
            for (std::size_t c = 0; c < C; ++c)
            {
                if (std::abs(actual[{ r, c }] - m_expected[{ r, c }]) >= epsilon)
                {
                    *listener << "at (" << r << ", " << c << "): expected " << m_expected[{ r, c }] << ", actual "
                              << actual[{ r, c }];
                    result = false;
                }
            }
        }

        return result;
    }

    void DescribeTo(::std::ostream* os) const { *os << "is approximately equal to " << m_expected; }

    void DescribeNegationTo(::std::ostream* os) const { *os << "is not approximately equal to " << m_expected; }
};

template <std::size_t D, class T>
struct ApproxEqualMatcher<zx::mat::vector_t<D, T>>
{
    using is_gtest_matcher = void;

    zx::mat::vector_t<D, T> m_expected;

    ApproxEqualMatcher(zx::mat::vector_t<D, T> expected) : m_expected{ expected } { }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::vector_t<D, U>& actual, ::testing::MatchResultListener* listener) const
    {
        constexpr auto epsilon = std::common_type_t<T, U>(0.1);

        bool result = true;

        for (std::size_t d = 0; d < D; ++d)
        {
            if (std::abs(actual[d] - m_expected[d]) >= epsilon)
            {
                *listener << "at (" << d << "): expected " << m_expected[d] << ", actual " << actual[d];
                result = false;
            }
        }

        return result;
    }

    void DescribeTo(::std::ostream* os) const { *os << "is approximately equal to " << m_expected; }

    void DescribeNegationTo(::std::ostream* os) const { *os << "is not approximately equal to " << m_expected; }
};

template <std::size_t D, class T>
struct ApproxEqualMatcher<zx::mat::segment_t<D, T>>
{
    using is_gtest_matcher = void;

    zx::mat::segment_t<D, T> m_expected;

    ApproxEqualMatcher(zx::mat::segment_t<D, T> expected) : m_expected{ expected } { }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::segment_t<D, U>& actual, ::testing::MatchResultListener* listener) const
    {
        constexpr auto epsilon = std::common_type_t<T, U>(0.1);

        bool result = true;

        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t d = 0; d < D; ++d)
            {
                if (std::abs(actual[i][d] - m_expected[i][d]) >= epsilon)
                {
                    *listener << "[" << i << "] at (" << d << "): expected " << m_expected[i][d] << ", actual "
                              << actual[i][d];
                    result = false;
                }
            }
        }

        return result;
    }

    void DescribeTo(::std::ostream* os) const { *os << "is approximately equal to " << m_expected; }

    void DescribeNegationTo(::std::ostream* os) const { *os << "is not approximately equal to " << m_expected; }
};

template <std::size_t D, std::size_t N, class T>
struct ApproxEqualMatcher<zx::mat::polygonal_shape_t<D, T, N>>
{
    using is_gtest_matcher = void;

    zx::mat::polygonal_shape_t<D, T, N> m_expected;

    ApproxEqualMatcher(zx::mat::polygonal_shape_t<D, T, N> expected) : m_expected{ expected } { }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::polygonal_shape_t<D, U, N>& actual, ::testing::MatchResultListener* listener) const
    {
        constexpr auto epsilon = std::common_type_t<T, U>(0.1);

        bool result = true;

        for (std::size_t n = 0; n < N; ++n)
        {
            for (std::size_t d = 0; d < D; ++d)
            {
                if (std::abs(actual[n][d] - m_expected[n][d]) >= epsilon)
                {
                    *listener << "[" << n << "] at (" << d << "): expected " << m_expected[n][d] << ", actual "
                              << actual[n][d];
                    result = false;
                }
            }
        }

        return result;
    }

    void DescribeTo(::std::ostream* os) const { *os << "is approximately equal to " << m_expected; }

    void DescribeNegationTo(::std::ostream* os) const { *os << "is not approximately equal to " << m_expected; }
};

template <class T>
struct ApproxEqualMatcher<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
    using is_gtest_matcher = void;

    T m_expected;

    ApproxEqualMatcher(T expected) : m_expected{ expected } { }

    template <class U = T>
    bool MatchAndExplain(U actual, ::testing::MatchResultListener* listener) const
    {
        constexpr auto epsilon = std::common_type_t<T, U>(0.1);

        if (std::abs(actual - m_expected) >= epsilon)
        {
            *listener << "expected " << m_expected << ", actual " << actual;
            return false;
        }

        return true;
    }

    void DescribeTo(::std::ostream* os) const { *os << "is approximately equal to " << m_expected; }

    void DescribeNegationTo(::std::ostream* os) const { *os << "is not approximately equal to " << m_expected; }
};

struct ApproxEqualFn
{
    template <class Expected>
    constexpr auto operator()(Expected expected) const
    {
        return ApproxEqualMatcher<Expected>{ std::move(expected) };
    }
};

}  // namespace detail

constexpr inline auto ApproxEqual = detail::ApproxEqualFn{};
