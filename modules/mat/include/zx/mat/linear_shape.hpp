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

    linear_shape_t(point_type p0, point_type p1) : base_t{ { p0, p1 } } { }
};

template <std::size_t D, class T, class Space = cartesian_space_t>
using line_t = linear_shape_t<D, detail::line_tag, T, Space>;

template <std::size_t D, class T, class Space = cartesian_space_t>
using ray_t = linear_shape_t<D, detail::ray_tag, T, Space>;

template <std::size_t D, class T, class Space = cartesian_space_t>
using segment_t = linear_shape_t<D, detail::segment_tag, T, Space>;

namespace detail
{

template <class Tag>
struct linear_shape_fn
{
    template <std::size_t D, class T, class Space>
    auto operator()(point_t<D, T, Space> p0, point_t<D, T, Space> p1) const -> linear_shape_t<D, Tag, T, Space>
    {
        return linear_shape_t<D, Tag, T, Space>{ p0, p1 };
    }
};

}  // namespace detail

inline constexpr auto line = detail::linear_shape_fn<detail::line_tag>{};
inline constexpr auto segment = detail::linear_shape_fn<detail::segment_tag>{};
inline constexpr auto ray = detail::linear_shape_fn<detail::ray_tag>{};

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
    return os << "(segment " << item[0] << " " << item[1] << ")";
}

template <std::size_t D, class T, class U, class Space>
constexpr bool operator==(const segment_t<D, T, Space>& lhs, const segment_t<D, U, Space>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <std::size_t D, class T, class U, class Space>
constexpr bool operator!=(const segment_t<D, T, Space>& lhs, const segment_t<D, U, Space>& rhs)
{
    return !(lhs == rhs);
}

}  // namespace mat

}  // namespace zx
