#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>

namespace zx
{

namespace mat
{

template <class T, std::size_t D>
struct vector;

template <class T, std::size_t D>
struct vector : public std::array<T, D>
{
    using base_t = std::array<T, D>;

    using base_t::base_t;

    constexpr vector() : base_t{}
    {
    }

    template <class... Tail>
    constexpr vector(T head, Tail... tail) : base_t{ head, static_cast<T>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == D, "Invalid number of arguments to vector constructor");
    }

    friend std::ostream& operator<<(std::ostream& os, const vector& item)
    {
        os << "[";
        for (std::size_t i = 0; i < D; ++i)
        {
            if (i != 0)
            {
                os << " ";
            }
            os << item[i];
        }
        os << "]";
        return os;
    }
};

template <class T>
vector(T, T) -> vector<T, 2>;

template <class T>
vector(T, T, T) -> vector<T, 3>;

template <class T, std::size_t D>
constexpr auto operator+(const vector<T, D>& item) -> vector<T, D>
{
    return item;
}

template <class T, std::size_t D>
constexpr auto operator-(const vector<T, D>& item) -> vector<T, D>
{
    vector<T, D> result;
    std::transform(std::begin(item), std::end(item), std::begin(result), std::negate<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(vector<L, D>& lhs, const vector<R, D>& rhs) -> vector<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(lhs), std::plus<>{});
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const vector<L, D>& lhs, const vector<R, D>& rhs) -> vector<Res, D>
{
    vector<Res, D> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(result), std::plus<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(vector<L, D>& lhs, const vector<R, D>& rhs) -> vector<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(lhs), std::minus<>{});
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const vector<L, D>& lhs, const vector<R, D>& rhs) -> vector<Res, D>
{
    vector<Res, D> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(result), std::minus<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(vector<L, D>& lhs, R rhs) -> vector<L, D>&
{
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(const vector<L, D>& lhs, R rhs) -> vector<Res, D>
{
    vector<Res, D> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(L lhs, const vector<R, D>& rhs) -> vector<Res, D>
{
    return rhs * lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(vector<L, D>& lhs, R rhs) -> vector<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(const vector<L, D>& lhs, R rhs) -> vector<Res, D>
{
    vector<Res, D> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class L, class R, std::size_t D, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const vector<L, D>& lhs, const vector<R, D>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <class L, class R, std::size_t D, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const vector<L, D>& lhs, const vector<R, D>& rhs)
{
    return !(lhs == rhs);
}

}  // namespace mat
}  // namespace zx
