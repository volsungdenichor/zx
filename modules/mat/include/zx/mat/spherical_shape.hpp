#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

template <std::size_t D, class T, class Space = cartesian_space_t>
struct spherical_shape_t
{
    point_t<D, T, Space> center;
    T radius;
};

template <class T, class Space = cartesian_space_t>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<2, T, Space>& item)
{
    return os << "(circle " << item.center << " " << item.radius << ")";
}

template <class T, class Space = cartesian_space_t>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<3, T, Space>& item)
{
    return os << "(sphere " << item.center << " " << item.radius << ")";
}

template <class T, class Space = cartesian_space_t>
using circle_t = spherical_shape_t<2, T, Space>;

template <class T, class Space = cartesian_space_t>
using sphere_t = spherical_shape_t<3, T, Space>;

namespace detail
{

struct circle_fn
{
    template <class T, class Space = cartesian_space_t>
    constexpr auto operator()(const point_t<2, T, Space>& center, T radius) const -> circle_t<T, Space>
    {
        return { center, radius };
    }
};

struct sphere_fn
{
    template <class T, class Space = cartesian_space_t>
    constexpr auto operator()(const point_t<3, T, Space>& center, T radius) const -> sphere_t<T, Space>
    {
        return { center, radius };
    }
};

}  // namespace detail

inline constexpr auto circle = detail::circle_fn{};
inline constexpr auto sphere = detail::sphere_fn{};

}  // namespace mat

}  // namespace zx
