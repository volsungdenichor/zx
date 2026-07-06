#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{

namespace mat
{

template <std::size_t D, class T, std::size_t N>
struct polygonal_shape_t : public std::array<vector_t<D, T>, N>
{
    using base_t = std::array<vector_t<D, T>, N>;

    using base_t::base_t;

    template <class... Tail>
    constexpr polygonal_shape_t(const vector_t<D, T>& head, Tail&&... tail) : base_t{ head, std::forward<Tail>(tail)... }
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

template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto translate(const polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) -> polygonal_shape_t<D, Res, N>
{
    return transform_into(polygonal_shape_t<D, Res, N>{}, std::bind(std::plus<>{}, std::placeholders::_1, rhs), lhs);
}

template <
    std::size_t D,
    std::size_t N,
    class T,
    std::size_t R,
    std::size_t C,
    class U,
    enable_if_t<(R == D + 1 && C == D + 1)> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto transform(const polygonal_shape_t<D, T, N>& lhs, const matrix_t<R, C, U>& rhs) -> polygonal_shape_t<D, Res, N>
{
    return transform_into(polygonal_shape_t<D, Res, N>{}, bind_back(std::multiplies<>{}, rhs), lhs);
}

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
struct vertex_list_shape_t : public std::vector<vector_t<D, T>>
{
    using base_t = std::vector<vector_t<D, T>>;

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

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto translate(const vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& offset)
    -> vertex_list_shape_t<D, Tag, Res>
{
    return transform_into(vertex_list_shape_t<D, Tag, Res>{}, std::bind(std::plus<>{}, std::placeholders::_1, offset), lhs);
}

template <
    std::size_t D,
    class Tag,
    class T,
    std::size_t R,
    std::size_t C,
    class U,
    enable_if_t<(R == D + 1 && C == D + 1)> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto transform(const vertex_list_shape_t<D, Tag, T>& lhs, const matrix_t<R, C, U>& transformation)
    -> vertex_list_shape_t<D, Tag, Res>
{
    vertex_list_shape_t<D, Tag, Res> result(lhs.size());
    return transform_into(std::move(result), std::bind(std::multiplies<>{}, std::placeholders::_1, transformation), lhs);
}

}  // namespace mat

}  // namespace zx
