#pragma once

#include <gmock/gmock.h>

#include <zx/mat.hpp>

namespace detail
{

template <class, class = void>
struct ApproxEqualMatcher;

template <class T, std::size_t R, std::size_t C>
struct ApproxEqualMatcher<zx::mat::matrix<T, R, C>>
{
    using is_gtest_matcher = void;

    zx::mat::matrix<T, R, C> m_expected;

    ApproxEqualMatcher(zx::mat::matrix<T, R, C> expected) : m_expected{ expected }
    {
    }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::matrix<U, R, C>& actual, ::testing::MatchResultListener* listener) const
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

    void DescribeTo(::std::ostream* os) const
    {
        *os << "is approximately equal to " << m_expected;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not approximately equal to " << m_expected;
    }
};

template <class T, std::size_t D>
struct ApproxEqualMatcher<zx::mat::vector<T, D>>
{
    using is_gtest_matcher = void;

    zx::mat::vector<T, D> m_expected;

    ApproxEqualMatcher(zx::mat::vector<T, D> expected) : m_expected{ expected }
    {
    }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::vector<U, D>& actual, ::testing::MatchResultListener* listener) const
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

    void DescribeTo(::std::ostream* os) const
    {
        *os << "is approximately equal to " << m_expected;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not approximately equal to " << m_expected;
    }
};

template <class T, std::size_t D>
struct ApproxEqualMatcher<zx::mat::segment<T, D>>
{
    using is_gtest_matcher = void;

    zx::mat::segment<T, D> m_expected;

    ApproxEqualMatcher(zx::mat::segment<T, D> expected) : m_expected{ expected }
    {
    }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::segment<U, D>& actual, ::testing::MatchResultListener* listener) const
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

    void DescribeTo(::std::ostream* os) const
    {
        *os << "is approximately equal to " << m_expected;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not approximately equal to " << m_expected;
    }
};

template <class T, std::size_t D, std::size_t N>
struct ApproxEqualMatcher<zx::mat::polygonal_shape<T, D, N>>
{
    using is_gtest_matcher = void;

    zx::mat::polygonal_shape<T, D, N> m_expected;

    ApproxEqualMatcher(zx::mat::polygonal_shape<T, D, N> expected) : m_expected{ expected }
    {
    }

    template <class U = T>
    bool MatchAndExplain(const zx::mat::polygonal_shape<U, D, N>& actual, ::testing::MatchResultListener* listener) const
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

    void DescribeTo(::std::ostream* os) const
    {
        *os << "is approximately equal to " << m_expected;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not approximately equal to " << m_expected;
    }
};

template <class T>
struct ApproxEqualMatcher<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
    using is_gtest_matcher = void;

    T m_expected;

    ApproxEqualMatcher(T expected) : m_expected{ expected }
    {
    }

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

    void DescribeTo(::std::ostream* os) const
    {
        *os << "is approximately equal to " << m_expected;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not approximately equal to " << m_expected;
    }
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
