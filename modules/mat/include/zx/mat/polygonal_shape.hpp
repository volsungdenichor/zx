#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{

namespace mat
{

template <class T, std::size_t D, std::size_t N>
struct polygonal_shape_t : public std::array<vector_t<T, D>, N>
{
    using base_t = std::array<vector_t<T, D>, N>;

    using base_t::base_t;

    template <class... Tail>
    constexpr polygonal_shape_t(const vector_t<T, D>& head, Tail&&... tail) : base_t{ head, std::forward<Tail>(tail)... }
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

template <class T, std::size_t D>
using triangle_t = polygonal_shape_t<T, D, 3>;

template <class T, std::size_t D>
using quad_t = polygonal_shape_t<T, D, 4>;

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator+=(polygonal_shape_t<T, D, N>& lhs, const vector_t<U, D>& rhs) -> polygonal_shape_t<T, D, N>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const polygonal_shape_t<T, D, N>& lhs, const vector_t<U, D>& rhs) -> polygonal_shape_t<Res, D, N>
{
    polygonal_shape_t<Res, D, N> result;
    std::transform(std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator-=(polygonal_shape_t<T, D, N>& lhs, const vector_t<U, D>& rhs) -> polygonal_shape_t<T, D, N>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const polygonal_shape_t<T, D, N>& lhs, const vector_t<U, D>& rhs) -> polygonal_shape_t<Res, D, N>
{
    polygonal_shape_t<Res, D, N> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N>
constexpr auto operator*=(polygonal_shape_t<T, D, N>& lhs, const matrix_t<U, D + 1>& rhs) -> polygonal_shape_t<T, D, N>&
{
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const polygonal_shape_t<T, D, N>& lhs, const matrix_t<U, D + 1>& rhs)
    -> polygonal_shape_t<Res, D, N>
{
    polygonal_shape_t<Res, D, N> result;
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const matrix_t<U, D + 1>& lhs, const polygonal_shape_t<T, D, N>& rhs)
    -> polygonal_shape_t<Res, D, N>
{
    return rhs * lhs;
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

template <class Tag, class T, std::size_t D>
struct vertex_list_shape_t : public std::vector<vector_t<T, D>>
{
    using base_t = std::vector<vector_t<T, D>>;

    using base_t::base_t;
};

template <class T, std::size_t D>
using polygon_t = vertex_list_shape_t<detail::polygon_tag, T, D>;

template <class T, std::size_t D>
using polyline_t = vertex_list_shape_t<detail::polyline_tag, T, D>;

template <class T, std::size_t D>
std::ostream& operator<<(std::ostream& os, const polygon_t<T, D>& item)
{
    os << "(polygon";
    for (std::size_t n = 0; n < item.size(); ++n)
    {
        os << " " << item[n];
    }
    return os << ")";
}

template <class T, std::size_t D>
std::ostream& operator<<(std::ostream& os, const polyline_t<T, D>& item)
{
    os << "(polyline";
    for (std::size_t n = 0; n < item.size(); ++n)
    {
        os << " " << item[n];
    }
    return os << ")";
}

template <class Tag, class T, class U, std::size_t D>
constexpr auto operator+=(vertex_list_shape_t<Tag, T, D>& lhs, const vector_t<U, D>& rhs) -> vertex_list_shape_t<Tag, T, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class Tag, class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const vertex_list_shape_t<Tag, T, D>& lhs, const vector_t<U, D>& rhs)
    -> vertex_list_shape_t<Tag, Res, D>
{
    vertex_list_shape_t<Tag, Res, D> result(lhs.size());
    std::transform(std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::plus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class Tag, class T, class U, std::size_t D>
constexpr auto operator-=(vertex_list_shape_t<Tag, T, D>& lhs, const vector_t<U, D>& rhs) -> vertex_list_shape_t<Tag, T, D>&
{
    std::transform(std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class Tag, class T, class U, std::size_t D, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const vertex_list_shape_t<Tag, T, D>& lhs, const vector_t<U, D>& rhs)
    -> vertex_list_shape_t<Tag, Res, D>
{
    vertex_list_shape_t<Tag, Res, D> result(lhs.size());
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::minus<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class Tag, class T, class U, std::size_t D>
constexpr auto operator*=(vertex_list_shape_t<Tag, T, D>& lhs, const matrix_t<U, D + 1>& rhs)
    -> vertex_list_shape_t<Tag, T, D>&
{
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(lhs), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class Tag, class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const vertex_list_shape_t<Tag, T, D>& lhs, const matrix_t<U, D + 1>& rhs)
    -> vertex_list_shape_t<Tag, Res, D>
{
    vertex_list_shape_t<Tag, Res, D> result(lhs.size());
    std::transform(
        std::begin(lhs), std::end(lhs), std::begin(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class Tag, class T, class U, std::size_t D, std::size_t N, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const matrix_t<U, D + 1>& lhs, const vertex_list_shape_t<Tag, T, D>& rhs)
    -> vertex_list_shape_t<Tag, Res, D>
{
    return rhs * lhs;
}

}  // namespace mat

}  // namespace zx
