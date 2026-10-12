#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

namespace detail
{

struct ray_tag
{
};
struct line_tag
{
};
struct segment_tag
{
};

}  // namespace detail

template <std::size_t D, class Tag, class T, class Space>
struct linear_shape_t : public std::array<point_t<D, T, Space>, 2>
{
    using base_t = std::array<point_t<D, T, Space>, 2>;
    using point_type = point_t<D, T, Space>;

    using base_t::base_t;

    constexpr linear_shape_t(point_type p0, point_type p1) : base_t{ { p0, p1 } } { }
};

template <std::size_t D, class T, class Space = cartesian_space_t>
using line_t = linear_shape_t<D, detail::line_tag, T, Space>;

template <std::size_t D, class T, class Space = cartesian_space_t>
using ray_t = linear_shape_t<D, detail::ray_tag, T, Space>;

template <std::size_t D, class T, class Space = cartesian_space_t>
using segment_t = linear_shape_t<D, detail::segment_tag, T, Space>;

template <std::size_t D, class T, class Space>
std::ostream& operator<<(std::ostream& os, const line_t<D, T, Space>& item)
{
    return os << "(line " << item[0] << " (dir " << (item[1] - item[0]) << "))";
}

template <std::size_t D, class T, class Space>
std::ostream& operator<<(std::ostream& os, const ray_t<D, T, Space>& item)
{
    return os << "(ray " << item[0] << " (dir " << (item[1] - item[0]) << "))";
}

template <std::size_t D, class T, class Space>
std::ostream& operator<<(std::ostream& os, const segment_t<D, T, Space>& item)
{
    return os << "[" << item[0] << " " << item[1] << "]";
}

}  // namespace mat

}  // namespace zx
