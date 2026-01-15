#pragma once

#include <gmock/gmock.h>

#include <zx/mat.hpp>

namespace detail
{

template <class>
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

        for (std::size_t r = 0; r < R; ++r)
        {
            for (std::size_t c = 0; c < C; ++c)
            {
                if (std::abs(actual[{ r, c }] - m_expected[{ r, c }]) >= epsilon)
                {
                    *listener << "at (" << r << ", " << c << "): expected " << m_expected[{ r, c }] << ", actual "
                              << actual[{ r, c }];
                    return false;
                }
            }
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
