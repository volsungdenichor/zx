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
struct vector_t;

template <class T, std::size_t D>
struct vector_t : public std::array<T, D>
{
    using base_t = std::array<T, D>;

    using base_t::base_t;

    constexpr vector_t() : base_t{} { std::fill(this->begin(), this->end(), T{}); }

    template <class... Tail>
    constexpr vector_t(T head, Tail... tail) : base_t{ head, static_cast<T>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == D, "Invalid number of arguments to vector_t constructor");
    }

    template <std::size_t D_ = D, std::enable_if_t<D_ == 1, int> = 0>
    constexpr operator T() const
    {
        return (*this)[0];
    }

    friend std::ostream& operator<<(std::ostream& os, const vector_t& item)
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
vector_t(T, T) -> vector_t<T, 2>;

template <class T>
vector_t(T, T, T) -> vector_t<T, 3>;

template <class T, std::size_t D>
constexpr auto operator+(const vector_t<T, D>& item) -> vector_t<T, D>
{
    return item;
}

template <class T, std::size_t D>
constexpr auto operator-(const vector_t<T, D>& item) -> vector_t<T, D>
{
    vector_t<T, D> result;
    std::transform(std::begin(item), std::end(item), std::begin(result), std::negate<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(vector_t<L, D>& lhs, const vector_t<R, D>& rhs) -> vector_t<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(lhs), std::plus<>{});
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const vector_t<L, D>& lhs, const vector_t<R, D>& rhs) -> vector_t<Res, D>
{
    vector_t<Res, D> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(result), std::plus<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(vector_t<L, D>& lhs, const vector_t<R, D>& rhs) -> vector_t<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(lhs), std::minus<>{});
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const vector_t<L, D>& lhs, const vector_t<R, D>& rhs) -> vector_t<Res, D>
{
    vector_t<Res, D> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(rhs), std::begin(result), std::minus<>{});
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(vector_t<L, D>& lhs, R rhs) -> vector_t<L, D>&
{
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(const vector_t<L, D>& lhs, R rhs) -> vector_t<Res, D>
{
    vector_t<Res, D> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(L lhs, const vector_t<R, D>& rhs) -> vector_t<Res, D>
{
    return rhs * lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(vector_t<L, D>& lhs, R rhs) -> vector_t<L, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class L, class R, std::size_t D, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(const vector_t<L, D>& lhs, R rhs) -> vector_t<Res, D>
{
    vector_t<Res, D> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class L, class R, std::size_t D, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const vector_t<L, D>& lhs, const vector_t<R, D>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <class L, class R, std::size_t D, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const vector_t<L, D>& lhs, const vector_t<R, D>& rhs)
{
    return !(lhs == rhs);
}

}  // namespace mat
}  // namespace zx
