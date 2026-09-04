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
    lhs[0] += rhs;
    lhs[1] += rhs;
    return lhs;
}

template <class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const interval_t<T>& lhs, U rhs) -> interval_t<Res>
{
    return interval_t<Res>{ lhs[0] + rhs, lhs[1] + rhs };
}

template <class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(T lhs, const interval_t<U>& rhs) -> interval_t<Res>
{
    return rhs + lhs;
}

template <class T, class U, class = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(interval_t<T>& lhs, U rhs) -> interval_t<T>&
{
    lhs[0] -= rhs;
    lhs[1] -= rhs;
    return lhs;
}

template <class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const interval_t<T>& lhs, U rhs) -> interval_t<Res>
{
    return interval_t<Res>{ lhs[0] - rhs, lhs[1] - rhs };
}

template <std::size_t D, class T>
struct box_shape_t : public md_base_t<D, interval_t<T>, box_shape_t>
{
    using base_t = md_base_t<D, interval_t<T>, box_shape_t>;
    using base_t::base_t;

    constexpr point_t<D, T> get(side_t side) const
    {
        return detail::map_into(
            point_t<D, T>{}, [&](const interval_t<T>& interval) -> T { return interval.get(side); }, *this);
    }

    constexpr point_t<D, T> get(const std::array<side_t, D>& sides) const
    {
        return detail::map_into(
            point_t<D, T>{},
            [&](const interval_t<T>& interval, side_t side) -> T { return interval.get(side); },
            *this,
            sides);
    }
};

struct interval
{
    template <class T>
    static constexpr interval_t<T> from_lower_upper(T lower, T upper)
    {
        return interval_t<T>{ lower, upper };
    }

    template <class T>
    static constexpr interval_t<T> from_lower_extent(T lower, T extent)
    {
        return interval_t<T>{ lower, lower + extent };
    }

    template <class T>
    static constexpr interval_t<T> from_center_extent(T center, T extent)
    {
        return from_lower_extent(center - extent / 2, extent);
    }

    template <class T>
    static constexpr interval_t<T> from_center_radius(T center, T left, T right)
    {
        if constexpr (std::is_integral_v<T>)
        {
            return from_lower_upper(center - left, center + right + 1);
        }
        else
        {
            return from_lower_upper(center - left, center + right);
        }
    }

    template <class T>
    static constexpr interval_t<T> from_center_radius(T center, T radius)
    {
        return from_center_radius(center, radius, radius);
    }
};

struct box
{
    template <std::size_t D, class T>
    static constexpr box_shape_t<D, T> from_lower_upper(const point_t<D, T>& lower, const point_t<D, T>& upper)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval::from_lower_upper(lower[d], upper[d]);
        }
        return result;
    }

    template <std::size_t D, class T>
    static constexpr box_shape_t<D, T> from_lower_extent(const point_t<D, T>& lower, const extent_t<D, T>& extent)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval::from_lower_extent(lower[d], extent[d]);
        }
        return result;
    }

    template <std::size_t D, class T>
    static constexpr box_shape_t<D, T> from_center_extent(const point_t<D, T>& center, const extent_t<D, T>& extent)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval::from_center_extent(center[d], extent[d]);
        }
        return result;
    }

    template <std::size_t D, class T>
    static constexpr box_shape_t<D, T> from_center_radius(
        const point_t<D, T>& center, const vector_t<D, T>& left, const vector_t<D, T>& right)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval::from_center_radius(center[d], left[d], right[d]);
        }
        return result;
    }

    template <std::size_t D, class T>
    static constexpr box_shape_t<D, T> from_center_radius(const point_t<D, T>& center, const vector_t<D, T>& radius)
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = interval::from_center_radius(center[d], radius[d]);
        }
        return result;
    }
};

template <class T>
using rectangle_t = box_shape_t<2, T>;

template <class T>
using cube_t = box_shape_t<3, T>;

}  // namespace mat
}  // namespace zx
