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

template <std::size_t D, class T, std::size_t N>
struct is_shape<polygonal_shape_t<D, T, N>> : public std::true_type
{
};

template <std::size_t D, class T>
using triangle_t = polygonal_shape_t<D, T, 3>;

template <std::size_t D, class T>
using quad_t = polygonal_shape_t<D, T, 4>;

template <std::size_t D, std::size_t N, class T, class U>
constexpr auto operator+=(polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) -> polygonal_shape_t<D, T, N>&
{
    return transform([&](const vector_t<D, T>& v) { return v + rhs; }, lhs);
}

template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) -> polygonal_shape_t<D, Res, N>
{
    return transform_into(polygonal_shape_t<D, Res, N>{}, std::bind(std::plus<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, std::size_t N, class T, class U>
constexpr auto operator-=(polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) -> polygonal_shape_t<D, T, N>&
{
    return transform(bind_back(std::minus<>{}, rhs), lhs);
}

template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) -> polygonal_shape_t<D, Res, N>
{
    return transform_into(polygonal_shape_t<D, Res, N>{}, bind_back(std::minus<>{}, rhs), lhs);
}

template <std::size_t D, std::size_t N, class T, class U>
constexpr auto operator*=(polygonal_shape_t<D, T, N>& lhs, const matrix_t<D + 1, D + 1, U>& rhs)
    -> polygonal_shape_t<D, T, N>&
{
    return transform(bind_back(std::multiplies<>{}, rhs), lhs);
}

template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const polygonal_shape_t<D, T, N>& lhs, const matrix_t<D + 1, D + 1, U>& rhs)
    -> polygonal_shape_t<D, Res, N>
{
    return transform_into(polygonal_shape_t<D, Res, N>{}, bind_back(std::multiplies<>{}, rhs), lhs);
}

template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix_t<D + 1, D + 1, U>& lhs, const polygonal_shape_t<D, T, N>& rhs)
    -> polygonal_shape_t<D, Res, N>
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

template <std::size_t D, class Tag, class T, class U>
constexpr auto operator+=(vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> vertex_list_shape_t<D, Tag, T>&
{
    return transform([&](const vector_t<D, T>& v) { return v + rhs; }, lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs)
    -> vertex_list_shape_t<D, Tag, Res>
{
    return transform_into(vertex_list_shape_t<D, Tag, Res>{}, std::bind(std::plus<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U>
constexpr auto operator-=(vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> vertex_list_shape_t<D, Tag, T>&
{
    return transform(std::bind(std::minus<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs)
    -> vertex_list_shape_t<D, Tag, Res>
{
    return transform_into(vertex_list_shape_t<D, Tag, Res>{}, std::bind(std::minus<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U>
constexpr auto operator*=(vertex_list_shape_t<D, Tag, T>& lhs, const matrix_t<D + 1, D + 1, U>& rhs)
    -> vertex_list_shape_t<D, Tag, T>&
{
    return transform(std::bind(std::multiplies<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const vertex_list_shape_t<D, Tag, T>& lhs, const matrix_t<D + 1, D + 1, U>& rhs)
    -> vertex_list_shape_t<D, Tag, Res>
{
    vertex_list_shape_t<D, Tag, Res> result(lhs.size());
    return transform_into(std::move(result), std::bind(std::multiplies<>{}, std::placeholders::_1, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator*(const matrix_t<D + 1, D + 1, U>& lhs, const vertex_list_shape_t<D, Tag, T>& rhs)
    -> vertex_list_shape_t<D, Tag, Res>
{
    return rhs * lhs;
}

}  // namespace mat

}  // namespace zx
