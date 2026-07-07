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

struct polygon_tag
{
};
struct polyline_tag
{
};

}  // namespace detail

template <std::size_t D, class Tag, class T>
struct vertex_list_shape_t : public std::vector<point_t<D, T>>
{
    using base_t = std::vector<point_t<D, T>>;

    using base_t::base_t;
};

template <std::size_t D, class T>
using polygon_t = vertex_list_shape_t<D, detail::polygon_tag, T>;

template <std::size_t D, class T>
using polyline_t = vertex_list_shape_t<D, detail::polyline_tag, T>;

template <std::size_t D, class T>
std::ostream& operator<<(std::ostream& os, const polygon_t<D, T>& item)
{
    os << "(polygon";
    for (std::size_t n = 0; n < item.size(); ++n)
    {
        os << " " << item[n];
    }
    return os << ")";
}

template <std::size_t D, class T>
std::ostream& operator<<(std::ostream& os, const polyline_t<D, T>& item)
{
    os << "(polyline";
    for (std::size_t n = 0; n < item.size(); ++n)
    {
        os << " " << item[n];
    }
    return os << ")";
}

}  // namespace mat

}  // namespace zx
