#pragma once

#include <zx/format.hpp>
#include <zx/mat.hpp>
#include <zx/test/functional_matcher.hpp>

namespace detail
{

struct ApproxEqualFn
{
    template <class Expected>
    constexpr auto operator()(const Expected& expected) const
    {
        return zx::test::FunctionalMatcher{ Comparer{ 0.1 }, Formatter{}, expected };
    }

    struct Formatter
    {
        template <class Expected>
        void operator()(std::ostream& os, bool positive, const Expected& e) const
        {
            if (positive)
            {
                os << "is approximately equal to " << e;
            }
            else
            {
                os << "is not approximately equal to " << e;
            }
        }
    };

    struct Comparer
    {
        double m_epsilon = 0.1;

        template <class T, class U, std::enable_if_t<std::is_floating_point_v<T> && std::is_floating_point_v<U>, int> = 0>
        auto operator()(T actual, U expected) const -> std::optional<std::string>
        {
            const auto epsilon = static_cast<std::common_type_t<T, U>>(m_epsilon);

            if (std::abs(actual - expected) >= epsilon)
            {
                return zx::format("expected ", expected, ", actual ", actual);
            }
            return std::nullopt;
        }

        template <std::size_t R, std::size_t C, class T, class U>
        auto operator()(const zx::mat::matrix_t<R, C, T>& actual, const zx::mat::matrix_t<R, C, U>& expected) const
            -> std::optional<std::string>
        {
            for (std::size_t r = 0; r < R; ++r)
            {
                for (std::size_t c = 0; c < C; ++c)
                {
                    if (auto res = (*this)(actual[{ r, c }], expected[{ r, c }]); res.has_value())
                    {
                        return zx::format("at (", r, ", ", c, "): ", *res);
                    }
                }
            }

            return std::nullopt;
        }

        template <std::size_t D, class T, class U>
        auto operator()(const zx::mat::vector_t<D, T>& actual, const zx::mat::vector_t<D, U>& expected) const
            -> std::optional<std::string>
        {
            for (std::size_t d = 0; d < D; ++d)
            {
                if (auto res = (*this)(actual[d], expected[d]); res.has_value())
                {
                    return zx::format("at (", d, "): ", *res);
                }
            }

            return std::nullopt;
        }

        template <std::size_t D, class T, class U>
        auto operator()(const zx::mat::segment_t<D, T>& actual, const zx::mat::segment_t<D, U>& expected) const
            -> std::optional<std::string>
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                if (auto res = (*this)(actual[i], expected[i]); res.has_value())
                {
                    return zx::format("[", i, "]: ", *res);
                }
            }

            return std::nullopt;
        }

        template <std::size_t D, std::size_t N, class T, class U>
        auto operator()(
            const zx::mat::polygonal_shape_t<D, T, N>& actual, const zx::mat::polygonal_shape_t<D, U, N>& expected) const
            -> std::optional<std::string>
        {
            for (std::size_t n = 0; n < N; ++n)
            {
                if (auto res = (*this)(actual[n], expected[n]); res.has_value())
                {
                    return zx::format("[", n, "]: ", *res);
                }
            }

            return std::nullopt;
        }
    };
};

}  // namespace detail

inline constexpr auto ApproxEqual = detail::ApproxEqualFn{};
