#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{

namespace mat
{

template <std::size_t D, class T, std::size_t N>
struct polygonal_shape_t : public std::array<point_t<D, T>, N>
{
    using base_t = std::array<point_t<D, T>, N>;

    using base_t::base_t;

    template <class... Tail>
    constexpr polygonal_shape_t(const point_t<D, T>& head, Tail&&... tail) : base_t{ head, std::forward<Tail>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == N, "Invalid number of arguments to polygonal_shape_t constructor");
    }

    friend std::ostream& operator<<(std::ostream& os, const polygonal_shape_t& item)
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

template <std::size_t D, class T>
using triangle_t = polygonal_shape_t<D, T, 3>;

template <std::size_t D, class T>
using quad_t = polygonal_shape_t<D, T, 4>;

namespace detail
{

struct triangle_fn
{
    template <std::size_t D, class T>
    constexpr auto operator()(const point_t<D, T>& p0, const point_t<D, T>& p1, const point_t<D, T>& p2) const
        -> triangle_t<D, T>
    {
        return triangle_t<D, T>{ p0, p1, p2 };
    }
};

struct quad_fn
{
    template <std::size_t D, class T>
    constexpr auto operator()(
        const point_t<D, T>& p0, const point_t<D, T>& p1, const point_t<D, T>& p2, const point_t<D, T>& p3) const
        -> quad_t<D, T>
    {
        return quad_t<D, T>{ p0, p1, p2, p3 };
    }
};

}  // namespace detail

inline constexpr auto triangle = detail::triangle_fn{};
inline constexpr auto quad = detail::quad_fn{};

template <std::size_t D, class T>
struct polygon_t : public std::vector<point_t<D, T>>
{
    using base_t = std::vector<point_t<D, T>>;

    using base_t::base_t;

    template <std::size_t N>
    polygon_t(const polygonal_shape_t<D, T, N>& polygonal_shape)
        : base_t(std::begin(polygonal_shape), std::end(polygonal_shape))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const polygon_t& item)
    {
        os << "(polygon";
        for (std::size_t n = 0; n < item.size(); ++n)
        {
            os << " " << item[n];
        }
        return os << ")";
    }
};

template <std::size_t D, class T>
struct polyline_t : public std::vector<point_t<D, T>>
{
    using base_t = std::vector<point_t<D, T>>;

    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const polyline_t& item)
    {
        os << "(polyline";
        for (std::size_t n = 0; n < item.size(); ++n)
        {
            os << " " << item[n];
        }
        return os << ")";
    }
};

namespace detail
{

struct polygon_fn
{
    template <std::size_t D, class T, class... Tail>
    constexpr auto operator()(const point_t<D, T>& head, Tail&&... tail) const -> polygon_t<D, T>
    {
        return polygon_t<D, T>{ head, std::forward<Tail>(tail)... };
    }
};

struct polyline_fn
{
    template <std::size_t D, class T, class... Tail>
    constexpr auto operator()(const point_t<D, T>& head, Tail&&... tail) const -> polyline_t<D, T>
    {
        return polyline_t<D, T>{ head, std::forward<Tail>(tail)... };
    }
};

}  // namespace detail

inline constexpr auto polygon = detail::polygon_fn{};
inline constexpr auto polyline = detail::polyline_fn{};

}  // namespace mat

}  // namespace zx
