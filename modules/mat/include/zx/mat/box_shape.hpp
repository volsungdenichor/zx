#pragma once

#include <zx/mat/vector.hpp>

namespace zx
{
namespace mat
{

template <class T>
struct interval_t : public std::array<T, 2>
{
    using base_t = std::array<T, 2>;
    using base_t::base_t;

    constexpr interval_t(T lo, T up) : base_t{ lo, up } { }

    constexpr interval_t() : interval_t(T{}, T{}) { }

    friend std::ostream& operator<<(std::ostream& os, const interval_t& item)
    {
        return os << "[" << item[0] << " " << item[1] << ")";
    }
};

template <class T, class U>
constexpr bool operator==(const interval_t<T>& lhs, const interval_t<U>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <class T, class U>
constexpr bool operator!=(const interval_t<T>& lhs, const interval_t<U>& rhs)
{
    return !(lhs == rhs);
}

template <class T, class U, class = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+=(interval_t<T>& lhs, U rhs) -> interval_t<T>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const interval_t<T>& lhs, U rhs) -> interval_t<Res>
{
    interval_t<Res> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(T lhs, const interval_t<U>& rhs) -> interval_t<Res>
{
    return rhs + lhs;
}

template <class T, class U, class = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(interval_t<T>& lhs, U rhs) -> interval_t<T>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const interval_t<T>& lhs, U rhs) -> interval_t<Res>
{
    interval_t<Res> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, std::size_t D>
struct box_shape_t : public std::array<interval_t<T>, D>
{
    using base_t = std::array<interval_t<T>, D>;

    using base_t::base_t;

    constexpr box_shape_t() : base_t{} { }

    template <class... Tail, class = std::enable_if_t<sizeof...(Tail) == D - 1>>
    constexpr box_shape_t(interval_t<T> head, Tail&&... tail)
        : base_t{ head, static_cast<interval_t<T>>(std::forward<Tail>(tail))... }
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const box_shape_t& item)
    {
        os << "(";
        for (std::size_t d = 0; d < D; ++d)
        {
            if (d != 0)
            {
                os << " ";
            }
            os << item[d];
        }
        os << ")";
        return os;
    }
};

template <class T, class U, std::size_t D>
constexpr bool operator==(const box_shape_t<T, D>& lhs, const box_shape_t<U, D>& rhs)
{
    for (std::size_t d = 0; d < D; ++d)
    {
        if (lhs[d] != rhs[d])
        {
            return false;
        }
    }
    return true;
}

template <class T, class U, std::size_t D>
constexpr bool operator!=(const box_shape_t<T, D>& lhs, const box_shape_t<U, D>& rhs)
{
    return !(lhs == rhs);
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+=(box_shape_t<T, D>& lhs, const vector<U, D>& rhs) -> box_shape_t<T, D>&
{
    for (std::size_t d = 0; d < D; ++d)
    {
        lhs[d] += rhs[d];
    }
    return lhs;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const box_shape_t<T, D>& lhs, const vector<U, D>& rhs) -> box_shape_t<Res, D>
{
    box_shape_t<Res, D> result;
    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = lhs[d] + rhs[d];
    }
    return result;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(box_shape_t<T, D>& lhs, const vector<U, D>& rhs) -> box_shape_t<T, D>&
{
    for (std::size_t d = 0; d < D; ++d)
    {
        lhs[d] -= rhs[d];
    }
    return lhs;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const box_shape_t<T, D>& lhs, const vector<U, D>& rhs) -> box_shape_t<Res, D>
{
    box_shape_t<Res, D> result;
    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = lhs[d] - rhs[d];
    }
    return result;
}

}  // namespace mat
}  // namespace zx
