#pragma once

#include <zx/mat/matrix.hpp>

#include "zx/mat/vector.hpp"

namespace zx
{

namespace mat
{

template <std::size_t D, class T, std::size_t N, class Space>
struct polygonal_shape_t : public std::array<point_t<D, T, Space>, N>
{
    using point_type = point_t<D, T, Space>;
    using base_t = std::array<point_type, N>;

    using base_t::base_t;

    template <class... Tail>
    constexpr polygonal_shape_t(const point_type& head, Tail&&... tail) : base_t{ head, std::forward<Tail>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == N, "Invalid number of arguments to polygonal_shape_t constructor");
    }

    friend std::ostream& operator<<(std::ostream& os, const polygonal_shape_t& item) { return detail::serialize(os, item); }
};

template <std::size_t D, class T, class Space = cartesian_space_t>
using triangle_t = polygonal_shape_t<D, T, 3, Space>;

template <std::size_t D, class T, class Space = cartesian_space_t>
using quad_t = polygonal_shape_t<D, T, 4, Space>;

namespace detail
{

struct triangle_fn
{
    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const point_t<D, T, Space>& p0, const point_t<D, T, Space>& p1, const point_t<D, T, Space>& p2)
        const -> triangle_t<D, T, Space>
    {
        return triangle_t<D, T, Space>{ p0, p1, p2 };
    }
};

struct quad_fn
{
    template <std::size_t D, class T, class Space>
    constexpr auto operator()(
        const point_t<D, T, Space>& p0,
        const point_t<D, T, Space>& p1,
        const point_t<D, T, Space>& p2,
        const point_t<D, T, Space>& p3) const -> quad_t<D, T, Space>
    {
        return quad_t<D, T, Space>{ p0, p1, p2, p3 };
    }
};

}  // namespace detail

inline constexpr auto triangle = detail::triangle_fn{};
inline constexpr auto quad = detail::quad_fn{};

template <std::size_t D, class T, class Space = cartesian_space_t>
struct polygon_t : public std::vector<point_t<D, T, Space>>
{
    using point_type = point_t<D, T, Space>;
    using base_t = std::vector<point_t<D, T, Space>>;

    using base_t::base_t;

    template <std::size_t N>
    polygon_t(const polygonal_shape_t<D, T, N, Space>& polygonal_shape)
        : base_t(std::begin(polygonal_shape), std::end(polygonal_shape))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const polygon_t& item) { return detail::serialize(os, item); }
};

template <std::size_t D, class T, class Space = cartesian_space_t>
struct polyline_t : public std::vector<point_t<D, T>>
{
    using base_t = std::vector<point_t<D, T>>;
    using point_type = point_t<D, T, Space>;

    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const polyline_t& item) { return detail::serialize(os, item); }
};

namespace detail
{

struct polygon_fn
{
    template <std::size_t D, class T, class Space, class... Tail>
    constexpr auto operator()(const point_t<D, T, Space>& head, Tail&&... tail) const -> polygon_t<D, T, Space>
    {
        return polygon_t<D, T, Space>{ head, std::forward<Tail>(tail)... };
    }
};

struct polyline_fn
{
    template <std::size_t D, class T, class Space, class... Tail>
    constexpr auto operator()(const point_t<D, T, Space>& head, Tail&&... tail) const -> polyline_t<D, T, Space>
    {
        return polyline_t<D, T, Space>{ head, std::forward<Tail>(tail)... };
    }
};

}  // namespace detail

inline constexpr auto polygon = detail::polygon_fn{};
inline constexpr auto polyline = detail::polyline_fn{};

}  // namespace mat

}  // namespace zx
