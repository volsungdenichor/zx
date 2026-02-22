#pragma once

#include <zx/mat/box_shape.hpp>
#include <zx/mat/linear_shape.hpp>
#include <zx/mat/math.hpp>
#include <zx/mat/matrix.hpp>
#include <zx/mat/polygonal_shape.hpp>
#include <zx/mat/spherical_shape.hpp>
#include <zx/mat/vector.hpp>

namespace zx
{

namespace mat
{
namespace detail
{

template <class T>
constexpr bool between(T v, T lo, T up)
{
    return lo <= v && v < up;
}

template <class T>
constexpr bool inclusive_between(T v, T lo, T up)
{
    return lo <= v && v <= up;
}

template <class T, class E>
constexpr auto approx_equal(T value, E epsilon)
{
    return [=](auto v) { return std::abs(v - value) < epsilon; };
}

template <class T, std::size_t D>
constexpr line_t<T, 2> make_line(const segment_t<T, D>& s)
{
    return line_t<T, 2>{ s[0], s[1] };
}

struct dot_fn
{
    template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector<T, D>& lhs, const vector<U, D>& rhs) const -> Res
    {
        return std::inner_product(std::begin(lhs), std::end(lhs), std::begin(rhs), Res{});
    }
};

static constexpr inline auto dot = dot_fn{};

struct cross_fn
{
    template <class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector<T, 2>& lhs, const vector<U, 2>& rhs) const -> Res
    {
        return lhs[0] * rhs[1] - lhs[1] * rhs[0];
    }

    template <class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector<T, 3>& lhs, const vector<U, 3>& rhs) const -> vector<Res, 3>
    {
        return vector<Res, 3>{ { lhs[1] * rhs[2] - lhs[2] * rhs[1],  //
                                 lhs[2] * rhs[0] - lhs[0] * rhs[2],
                                 lhs[0] * rhs[1] - lhs[1] * rhs[0] } };
    }
};

static constexpr inline auto cross = cross_fn{};

struct angle_fn
{
    template <class T>
    constexpr auto operator()(const vector<T, 2>& lhs, const vector<T, 2>& rhs) const
        -> decltype(math::atan2(cross(lhs, rhs), dot(lhs, rhs)))
    {
        return math::atan2(cross(lhs, rhs), dot(lhs, rhs));
    }

    template <class T>
    constexpr auto operator()(const vector<T, 3>& lhs, const vector<T, 3>& rhs) const
        -> decltype(math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs))))
    {
        return math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs)));
    }
};

static constexpr inline auto angle = angle_fn{};

struct norm_fn
{
    template <class T, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, T, T>>
    constexpr auto operator()(const vector<T, D>& item) const -> Res
    {
        return dot(item, item);
    }
};

static constexpr inline auto norm = norm_fn{};

struct length_fn
{
    template <class T, std::size_t D>
    constexpr auto operator()(const vector<T, D>& item) const -> decltype(math::sqrt(norm(item)))
    {
        return math::sqrt(norm(item));
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const segment_t<T, D>& item) const
    {
        return (*this)(item[1] - item[0]);
    }
};

static constexpr inline auto length = length_fn{};

struct unit_fn
{
    template <
        class T,
        std::size_t D,
        class Sqr = std::invoke_result_t<std::multiplies<>, T, T>,
        class Sqrt = decltype(sqrt(std::declval<Sqr>())),
        class Res = std::invoke_result_t<std::divides<>, T, Sqrt>>
    constexpr auto operator()(const vector<T, D>& item) const -> vector<Res, D>
    {
        const auto len = length(item);
        return len ? item / len : item;
    }
};

static constexpr inline auto unit = unit_fn{};

struct distance_fn
{
    template <class T, class U, std::size_t D>
    constexpr auto operator()(const vector<T, D>& lhs, const vector<U, D>& rhs) const -> decltype(length(rhs - lhs))
    {
        return length(rhs - lhs);
    }
};

static constexpr inline auto distance = distance_fn{};

template <std::size_t Dim>
struct lower_upper_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return item[Dim];
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& item) const -> vector<T, D>
    {
        vector<T, D> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d]);
        }
        return result;
    }
};

static constexpr inline auto lower = lower_upper_fn<0>{};
static constexpr inline auto upper = lower_upper_fn<1>{};

template <std::size_t Dim>
struct min_max_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        if constexpr (Dim == 0)
        {
            return item[0];
        }
        else
        {
            if constexpr (std::is_integral_v<T>)
            {
                return item[1] - 1;
            }
            else
            {
                return item[1] - std::numeric_limits<T>::epsilon();
            }
        }
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& item) const -> vector<T, D>
    {
        vector<T, D> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d]);
        }
        return result;
    }
};

static constexpr inline auto min = min_max_fn<0>{};
static constexpr inline auto max = min_max_fn<1>{};

struct size_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return upper(item) - lower(item);
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& item) const -> vector<T, D>
    {
        return upper(item) - lower(item);
    }
};

static constexpr inline auto size = size_fn{};

struct center_fn
{
    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& item) const -> vector<T, D>
    {
        return (lower(item) + upper(item)) / 2;
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const segment_t<T, D>& item) const -> vector<T, D>
    {
        return (item[0] + item[1]) / 2;
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const spherical_shape_t<T, D>& item) const -> vector<T, D>
    {
        return item.center;
    }
};

static constexpr inline auto center = center_fn{};

struct orientation_fn
{
    template <class T, class U>
    constexpr auto operator()(const vector<T, 2>& point, const vector<U, 2>& start, const vector<U, 2>& end) const
    {
        return cross(end - start, point - start);
    }

    template <class T, class U, class Tag>
    constexpr auto operator()(const vector<T, 2>& point, const linear_shape_t<Tag, U, 2>& shape) const
    {
        return (*this)(point, shape[0], shape[1]);
    }
};

static constexpr inline auto orientation = orientation_fn{};

struct contains_fn
{
    template <class T, class U>
    constexpr auto operator()(const interval_t<T>& item, U value) const -> bool
    {
        return between(value, lower(item), upper(item));
    }

    template <class T>
    constexpr auto operator()(const interval_t<T>& item, const interval_t<T>& other) const -> bool
    {
        constexpr T lo = lower(item);
        constexpr T up = upper(item);
        return inclusive_between(lower(other), lo, up) && inclusive_between(upper(other), lo, up);
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& item, const box_shape_t<T, D>& other) const -> bool
    {
        for (std::size_t d = 0; d < D; ++d)
        {
            if (!(*this)(item[d], other[d]))
            {
                return false;
            }
        }
        return true;
    }

    template <class T, class U, std::size_t D>
    constexpr auto operator()(const spherical_shape_t<T, D>& item, const vector<U, D>& other) const -> bool
    {
        return norm(other - center(item)) <= math::sqr(item.radius);
    }

    template <class T, class U>
    constexpr bool operator()(const triangle_t<T, 2>& item, const vector<U, 2>& other) const
    {
        constexpr auto same_sign = [](int a, int b) { return (a <= 0 && b <= 0) || (a >= 0 && b >= 0); };

        int result[3] = { 0, 0, 0 };

        for (std::size_t i = 0; i < 3; ++i)
        {
            result[i] = math::sign(orientation(other, segment_t<T, 2>{ item[(i + 0) % 3], item[(i + 1) % 3] }));
        }

        return same_sign(result[0], result[1]) && same_sign(result[0], result[2]) && same_sign(result[1], result[2]);
    }
};

static constexpr inline auto contains = contains_fn{};

struct intersects_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& self, const interval_t<T>& other) const -> bool
    {
        return inclusive_between(lower(self), lower(other), upper(other))     //
               || inclusive_between(upper(self), lower(other), upper(other))  //
               || inclusive_between(lower(other), lower(self), upper(self))   //
               || inclusive_between(upper(other), lower(self), upper(self));
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const box_shape_t<T, D>& self, const box_shape_t<T, D>& other) const -> bool
    {
        for (std::size_t d = 0; d < D; ++d)
        {
            if (!(*this)(self[d], other[d]))
            {
                return false;
            }
        }
        return true;
    }
};

static constexpr inline auto intersects = intersects_fn{};

struct interpolate_fn
{
    template <class R, class T, std::size_t D>
    constexpr auto operator()(R r, const vector<T, D>& lhs, const vector<T, D>& rhs) const -> vector<T, D>
    {
        return lhs + r * (rhs - lhs);
    }

    template <class R, class T, std::size_t D>
    constexpr auto operator()(R r, const segment_t<T, D>& value) const
    {
        return (*this)(r, value[0], value[1]);
    }

    template <class R, class T>
    constexpr auto operator()(R r, const interval_t<T>& item) const
    {
        return lower(item) * r + upper(item) * (R(1) - r);
    }
};

static constexpr inline auto interpolate = interpolate_fn{};

namespace detail
{

template <class T, class E>
constexpr auto get_line_intersection_parameter(
    const vector<T, 2>& a0, const vector<T, 2>& a1, const vector<T, 2>& p, E epsilon) -> std::optional<T>
{
    const auto dir = a1 - a0;

    const auto d = p - a0;

    const auto det = cross(dir, d);
    if (!(approx_equal(E(0), epsilon)(det)))
    {
        return {};
    }

    return dot(d, dir) / norm(dir);
}

template <class T, class E>
constexpr auto get_line_intersection_parameters(
    const vector<T, 2>& a0, const vector<T, 2>& a1, const vector<T, 2>& b0, const vector<T, 2>& b1, E epsilon)
    -> std::optional<std::tuple<T, T>>
{
    const auto dir_a = a1 - a0;
    const auto dir_b = b1 - b0;

    const auto det = cross(dir_a, dir_b);

    if (approx_equal(E(0), epsilon)(det))
    {
        return {};
    }
    const auto v = b0 - a0;

    return { { cross(v, dir_b) / det, cross(v, dir_a) / det } };
}

template <class T>
constexpr bool contains_param(line_tag, T)
{
    return true;
}

template <class T>
constexpr bool contains_param(ray_tag, T v)
{
    return v >= T(0);
}

template <class T>
constexpr bool contains_param(segment_tag, T v)
{
    return T(0) <= v && v <= T(1);
}

}  // namespace detail

struct intersection_fn
{
    template <class T, std::size_t D, class Tag1, class Tag2, class E = T>
    constexpr auto operator()(const linear_shape_t<Tag1, T, D>& lhs, const linear_shape_t<Tag2, T, D>& rhs, E epsilon = {})
        const -> std::optional<vector<T, D>>
    {
        const auto par = detail::get_line_intersection_parameters(lhs[0], lhs[1], rhs[0], rhs[1], epsilon);

        if (!par)
        {
            return {};
        }

        const auto [a, b] = *par;

        if (detail::contains_param(Tag1{}, a) && detail::contains_param(Tag2{}, b))
        {
            return interpolate(a, lhs[0], lhs[1]);
        }
        return {};
    }
};

static constexpr inline auto intersection = intersection_fn{};

struct projection_fn
{
    template <class T, std::size_t D>
    constexpr auto operator()(const vector<T, D>& lhs, const vector<T, D>& rhs) const
        -> decltype(rhs * (dot(rhs, lhs) / norm(rhs)))
    {
        return rhs * (dot(rhs, lhs) / norm(rhs));
    }

    template <class T, std::size_t D, class Tag, class E = T>
    constexpr auto operator()(const vector<T, D>& point, const linear_shape_t<Tag, T, D>& shape, E epsilon = {}) const
        -> std::optional<vector<T, D>>
    {
        const auto p0 = shape[0];
        const auto p1 = shape[1];

        const auto result = p0 + (*this)(point - p0, p1 - p0);

        const auto t = detail::get_line_intersection_parameter(p0, p1, result, epsilon);

        if (t && detail::contains_param(Tag{}, *t))
        {
            return result;
        }

        return {};
    }
};

static constexpr inline auto projection = projection_fn{};

struct rejection_fn
{
    template <class T, std::size_t D>
    constexpr auto operator()(const vector<T, D>& lhs, const vector<T, D>& rhs) const -> decltype(lhs - projection(lhs, rhs))
    {
        return lhs - projection(lhs, rhs);
    }
};

static constexpr inline auto rejection = rejection_fn{};

struct perpendicular_fn
{
    template <class T>
    constexpr auto operator()(const vector<T, 2>& value) const -> vector<T, 2>
    {
        return vector<T, 2>{ -value[1], value[0] };
    }

    template <class Tag, class T>
    constexpr auto operator()(const linear_shape_t<Tag, T, 2>& value, const vector<T, 2>& origin) const
        -> linear_shape_t<Tag, T, 2>
    {
        return { origin, origin + (*this)(value[1] - value[0]) };
    }

    template <class Tag, class T>
    constexpr auto operator()(const linear_shape_t<Tag, T, 2>& value) const -> linear_shape_t<Tag, T, 2>
    {
        return (*this)(value, value[0]);
    }
};

static constexpr inline auto perpendicular = perpendicular_fn{};

struct altitude_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<T, 2>& value, std::size_t index) const -> segment_t<T, 2>
    {
        constexpr T epsilon = T(0.1);

        const auto v = value[(index + 0) % 3];

        const auto p = projection(v, line_t<T, 2>{ value[(index + 1) % 3], value[(index + 2) % 3] }, epsilon);

        return { v, *p };
    }
};

static constexpr inline auto altitude = altitude_fn{};

struct centroid_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<T, 2>& value) const -> vector<T, 2>
    {
        return std::accumulate(std::begin(value), std::end(value), vector<T, 2>{}) / 3;
    }
};

static constexpr inline auto centroid = centroid_fn{};

struct orthocenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<T, 2>& value) const -> vector<T, 2>
    {
        constexpr T epsilon = T(0.0001);

        return *intersection(make_line(altitude(value, 0)), make_line(altitude(value, 1)), epsilon);
    }
};

static constexpr inline auto orthocenter = orthocenter_fn{};

struct circumcenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<T, 2>& value) const -> vector<T, 2>
    {
        constexpr T epsilon = T(0.0001);

        const auto s0 = segment_t<T, 2>{ value[0], value[1] };
        const auto s1 = segment_t<T, 2>{ value[1], value[2] };

        return *intersection(make_line(perpendicular(s0, center(s0))), make_line(perpendicular(s1, center(s1))), epsilon);
    }
};

static constexpr inline auto circumcenter = circumcenter_fn{};

struct incenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<T, 2>& value) const -> vector<T, 2>
    {
        T perimeter = T(0);
        std::array<T, 3> sides = {};
        for (std::size_t i = 0; i < 3; ++i)
        {
            sides[i] = length(segment_t<T, 2>{ value[(i + 1) % 3], value[(i + 2) % 3] });
            perimeter += sides[i];
        }

        vector<T, 2> result = {};

        for (std::size_t i = 0; i < 3; ++i)
        {
            result += sides[i] * value[i];
        }
        result /= perimeter;

        return result;
    }
};

static constexpr inline auto incenter = incenter_fn{};

struct incircle_fn
{
    template <class T>
    constexpr auto operator()(const triangle_t<T, 2>& triangle_t) const -> spherical_shape_t<T, 2>
    {
        constexpr T epsilon = T(0.1);

        const auto c = incenter(triangle_t);
        const auto r = distance(c, *projection(c, segment_t<T, 2>{ triangle_t[0], triangle_t[1] }, epsilon));

        return spherical_shape_t<T, 2>{ c, static_cast<T>(r) };
    }
};

static constexpr inline auto incircle = incircle_fn{};

struct circumcircle_fn
{
    template <class T>
    constexpr auto operator()(const triangle_t<T, 2>& triangle_t) const -> spherical_shape_t<T, 2>
    {
        const auto c = circumcenter(triangle_t);
        const auto r = distance(c, triangle_t[0]);

        return spherical_shape_t<T, 2>{ c, static_cast<T>(r) };
    }
};

static constexpr inline auto circumcircle = circumcircle_fn{};

}  // namespace detail

using detail::altitude;
using detail::angle;
using detail::center;
using detail::centroid;
using detail::circumcenter;
using detail::circumcircle;
using detail::contains;
using detail::cross;
using detail::distance;
using detail::dot;
using detail::incenter;
using detail::incircle;
using detail::interpolate;
using detail::intersection;
using detail::intersects;
using detail::length;
using detail::lower;
using detail::max;
using detail::min;
using detail::norm;
using detail::orthocenter;
using detail::perpendicular;
using detail::projection;
using detail::rejection;
using detail::size;
using detail::unit;
using detail::upper;

}  // namespace mat

}  // namespace zx
