#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{

namespace mat
{

template <class T, std::size_t D, std::size_t N>
struct polygonal_shape : public std::array<vector<T, D>, N>
{
    using base_t = std::array<vector<T, D>, N>;

    using base_t::base_t;

    template <class... Tail>
    constexpr polygonal_shape(const vector<T, D>& head, Tail&&... tail) : base_t{ head, std::forward<Tail>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == N, "Invalid number of arguments to polygonal_shape constructor");
    }

    friend std::ostream& operator<<(std::ostream& os, const polygonal_shape& item)
    {
        os << "(";
        for (std::size_t n = 0; n < item.size(); ++n)
        {
            if (n != 0)
            {
                os << " ";
            }
            os << item[n];
        }
        os << ")";
        return os;
    }
};

template <class T, std::size_t D>
using triangle = polygonal_shape<T, D, 3>;

template <class T, std::size_t D>
using quad = polygonal_shape<T, D, 4>;

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator+=(polygonal_shape<T, D, N>& lhs, const vector<U, D>& rhs) -> polygonal_shape<T, D, N>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const polygonal_shape<T, D, N>& lhs, const vector<U, D>& rhs) -> polygonal_shape<Res, D, N>
{
    polygonal_shape<Res, D, N> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator-=(polygonal_shape<T, D, N>& lhs, const vector<U, D>& rhs) -> polygonal_shape<T, D, N>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const polygonal_shape<T, D, N>& lhs, const vector<U, D>& rhs) -> polygonal_shape<Res, D, N>
{
    polygonal_shape<Res, D, N> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator*=(polygonal_shape<T, D, N>& lhs, const matrix<U, D + 1>& rhs) -> polygonal_shape<T, D, N>&
{
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const polygonal_shape<T, D, N>& lhs, const matrix<U, D + 1>& rhs) -> polygonal_shape<Res, D, N>
{
    polygonal_shape<Res, D, N> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const matrix<U, D + 1>& lhs, const polygonal_shape<T, D, N>& rhs) -> polygonal_shape<Res, D, N>
{
    return rhs * lhs;
}

}  // namespace mat

}  // namespace zx
