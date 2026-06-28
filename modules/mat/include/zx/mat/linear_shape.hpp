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

template <std::size_t D, class Tag, class T>
struct linear_shape_t : public std::array<vector_t<D, T>, 2>
{
    using base_t = std::array<vector_t<D, T>, 2>;

    using base_t::base_t;

    linear_shape_t(vector_t<D, T> p0, vector_t<D, T> p1) : base_t{ { p0, p1 } } { }
};

template <std::size_t D, class Tag, class T>
struct is_shape<linear_shape_t<D, Tag, T>> : public std::true_type
{
};

template <std::size_t D, class T>
using line_t = linear_shape_t<D, detail::line_tag, T>;

template <std::size_t D, class T>
using ray_t = linear_shape_t<D, detail::ray_tag, T>;

template <std::size_t D, class T>
using segment_t = linear_shape_t<D, detail::segment_tag, T>;

namespace detail
{

struct line_fn
{
    template <std::size_t D, class T>
    auto operator()(vector_t<D, T> p0, vector_t<D, T> p1) const -> line_t<D, T>
    {
        return line_t<D, T>{ p0, p1 };
    }
};

struct segment_fn
{
    template <std::size_t D, class T>
    auto operator()(vector_t<D, T> p0, vector_t<D, T> p1) const -> segment_t<D, T>
    {
        return segment_t<D, T>{ p0, p1 };
    }
};

struct ray_fn
{
    template <std::size_t D, class T>
    auto operator()(vector_t<D, T> p0, vector_t<D, T> p1) const -> ray_t<D, T>
    {
        return ray_t<D, T>{ p0, p1 };
    }
};

}  // namespace detail

static constexpr inline auto line = detail::line_fn{};
static constexpr inline auto segment = detail::segment_fn{};
static constexpr inline auto ray = detail::ray_fn{};

template <std::size_t D, class T>
std::ostream& operator<<(std::ostream& os, const line_t<D, T>& item)
{
    return os << "(line " << item[0] << " (dir " << (item[1] - item[0]) << "))";
}

template <std::size_t D, class T>
std::ostream& operator<<(std::ostream& os, const ray_t<D, T>& item)
{
    return os << "(ray " << item[0] << " (dir " << (item[1] - item[0]) << "))";
}

template <std::size_t D, class T>
std::ostream& operator<<(std::ostream& os, const segment_t<D, T>& item)
{
    return os << "(segment " << item[0] << " " << item[1] << ")";
}

template <std::size_t D, class Tag, class T, class U>
constexpr auto operator+=(linear_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> linear_shape_t<D, Tag, T>&
{
    return transform(bind_back(std::plus<>{}, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const linear_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> linear_shape_t<D, Tag, Res>
{
    return transform_into(linear_shape_t<D, Tag, Res>{}, bind_back(std::plus<>{}, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U>
constexpr auto operator-=(linear_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> linear_shape_t<D, Tag, T>&
{
    return transform(bind_back(std::minus<>{}, rhs), lhs);
}

template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const linear_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& rhs) -> linear_shape_t<D, Tag, Res>
{
    return transform_into(linear_shape_t<D, Tag, Res>{}, bind_back(std::minus<>{}, rhs), lhs);
}

template <
    std::size_t D,
    class Tag,
    class T,
    std::size_t R,
    std::size_t C,
    class U,
    enable_if_t<
        R == D + 1,
        C == D + 1,
        std::is_invocable_v<std::multiplies<>, const vector_t<D, T>&, const matrix_t<R, C, U>&>> = 0>
constexpr auto operator*=(linear_shape_t<D, Tag, T>& lhs, const matrix_t<R, C, U>& rhs) -> linear_shape_t<D, Tag, T>&
{
    return transform(bind_back(std::multiplies<>{}, rhs), lhs);
}

template <
    std::size_t D,
    class Tag,
    class T,
    std::size_t R,
    std::size_t C,
    class U,
    enable_if_t<
        R == D + 1,
        C == D + 1,
        std::is_invocable_v<std::multiplies<>, const vector_t<D, T>&, const matrix_t<R, C, U>&>> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const linear_shape_t<D, Tag, T>& lhs, const matrix_t<R, C, U>& rhs)
    -> linear_shape_t<D, Tag, Res>
{
    return transform_into(linear_shape_t<D, Tag, Res>{}, bind_back(std::multiplies<>{}, rhs), lhs);
}

template <std::size_t D, class T, class U>
constexpr bool operator==(const segment_t<D, T>& lhs, const segment_t<D, U>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <std::size_t D, class T, class U>
constexpr bool operator!=(const segment_t<D, T>& lhs, const segment_t<D, U>& rhs)
{
    return !(lhs == rhs);
}

}  // namespace mat

}  // namespace zx
