#pragma once

#include <zx/mat/vector.hpp>

namespace zx
{
namespace mat
{

enum class side_t
{
    lower,
    upper,
    first,
    last,
    middle
};

template <class T>
struct interval_t : public std::array<T, 2>
{
    using base_t = std::array<T, 2>;
    using base_t::base_t;

    constexpr interval_t(T lo, T up) : base_t{ lo, up } { }

    constexpr interval_t() : interval_t(T{}, T{}) { }

    static constexpr interval_t<T> from_lower_upper(T lower, T upper) { return interval_t<T>{ lower, upper }; }

    static constexpr interval_t<T> from_lower_size(T lower, T size) { return interval_t<T>{ lower, lower + size }; }

    static constexpr interval_t<T> from_center_size(T center, T size)
    {
        return interval_t<T>{ center - size / 2, center + size / 2 };
    }

    constexpr T get(side_t s) const
    {
        switch (s)
        {
            case side_t::lower: return (*this)[0];
            case side_t::upper: return (*this)[1];
            case side_t::first: return (*this)[0];
            case side_t::last:
                if constexpr (std::is_integral_v<T>)
                {
                    return (*this)[1] - 1;
                }
                else
                {
                    return (*this)[1];
                }
            case side_t::middle: return ((*this)[0] + (*this)[1]) / 2;
            default: return (*this)[0];
        }
    }

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

template <std::size_t D, class T>
struct box_shape_t : public md_base_t<D, interval_t<T>, box_shape_t>
{
    using base_t = md_base_t<D, interval_t<T>, box_shape_t>;
    using base_t::base_t;

    static constexpr box_shape_t<D, T> from_lower_upper(const vector_t<D, T>& lower, const vector_t<D, T>& upper)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval_t<T>::from_lower_upper(lower[d], upper[d]);
        }
        return result;
    }

    static constexpr box_shape_t<D, T> from_lower_size(const vector_t<D, T>& lower, const vector_t<D, T>& size)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval_t<T>::from_lower_size(lower[d], size[d]);
        }
        return result;
    }

    static constexpr box_shape_t<D, T> from_center_size(const vector_t<D, T>& center, const vector_t<D, T>& size)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval_t<T>::from_center_size(center[d], size[d]);
        }
        return result;
    }

    constexpr vector_t<D, T> get(side_t s) const
    {
        return vector_t<D, T>{ this->transform([&](const interval_t<T>& interval) -> T { return interval.get(s); }) };
    }

    constexpr vector_t<D, T> get(const std::array<side_t, D>& s) const
    {
        vector_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)[d].get(s[d]);
        }
        return result;
    }
};

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+=(box_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> box_shape_t<D, T>&
{
    for (std::size_t d = 0; d < D; ++d)
    {
        lhs[d] += rhs[d];
    }
    return lhs;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const box_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> box_shape_t<D, Res>
{
    box_shape_t<D, Res> result;
    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = lhs[d] + rhs[d];
    }
    return result;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(box_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> box_shape_t<D, T>&
{
    for (std::size_t d = 0; d < D; ++d)
    {
        lhs[d] -= rhs[d];
    }
    return lhs;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const box_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> box_shape_t<D, Res>
{
    box_shape_t<D, Res> result;
    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = lhs[d] - rhs[d];
    }
    return result;
}

}  // namespace mat
}  // namespace zx
