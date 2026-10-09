#pragma once

#include <zx/mat/box_shape.hpp>
#include <zx/mat/linear_shape.hpp>
#include <zx/mat/math.hpp>
#include <zx/mat/matrix.hpp>
#include <zx/mat/polygonal_shape.hpp>
#include <zx/mat/spherical_shape.hpp>
#include <zx/mat/vector.hpp>
#include <zx/maybe.hpp>
#include <zx/sequence.hpp>

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

template <std::size_t D, class T, class Space>
constexpr line_t<2, T, Space> make_line(const segment_t<D, T, Space>& s)
{
    return line_t<2, T, Space>{ s[0], s[1] };
}

struct dot_fn
{
    template <std::size_t D, class T, class U, class Space, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<D, T, Space>& lhs, const vector_t<D, U, Space>& rhs) const -> Res
    {
        return std::inner_product(std::begin(lhs), std::end(lhs), std::begin(rhs), Res{});
    }
};

inline constexpr auto dot = dot_fn{};

struct cross_fn
{
    template <class T, class U, class Space, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<2, T, Space>& lhs, const vector_t<2, U, Space>& rhs) const -> Res
    {
        return lhs[0] * rhs[1] - lhs[1] * rhs[0];
    }

    template <class T, class U, class Space, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<3, T, Space>& lhs, const vector_t<3, U, Space>& rhs) const
        -> vector_t<3, Res, Space>
    {
        return vector_t<3, Res, Space>{ { lhs[1] * rhs[2] - lhs[2] * rhs[1],  //
                                          lhs[2] * rhs[0] - lhs[0] * rhs[2],
                                          lhs[0] * rhs[1] - lhs[1] * rhs[0] } };
    }
};

inline constexpr auto cross = cross_fn{};

struct angle_fn
{
    template <class T, class Space>
    constexpr auto operator()(const vector_t<2, T, Space>& lhs, const vector_t<2, T, Space>& rhs) const
        -> decltype(math::atan2(cross(lhs, rhs), dot(lhs, rhs)))
    {
        return math::atan2(cross(lhs, rhs), dot(lhs, rhs));
    }

    template <class T, class Space>
    constexpr auto operator()(const vector_t<3, T, Space>& lhs, const vector_t<3, T, Space>& rhs) const
        -> decltype(math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs))))
    {
        return math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs)));
    }
};

inline constexpr auto angle = angle_fn{};

struct norm_fn
{
    template <std::size_t D, class T, class Space, class Res = std::invoke_result_t<std::multiplies<>, T, T>>
    constexpr auto operator()(const vector_t<D, T, Space>& item) const -> Res
    {
        return dot(item, item);
    }
};

inline constexpr auto norm = norm_fn{};

struct length_fn
{
    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const vector_t<D, T, Space>& item) const -> decltype(math::sqrt(norm(item)))
    {
        return math::sqrt(norm(item));
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const segment_t<D, T, Space>& item) const
    {
        return (*this)(item[1] - item[0]);
    }
};

inline constexpr auto length = length_fn{};

struct unit_fn
{
    template <
        std::size_t D,
        class T,
        class Space,
        class Sqr = std::invoke_result_t<std::multiplies<>, T, T>,
        class Sqrt = decltype(sqrt(std::declval<Sqr>())),
        class Res = std::invoke_result_t<std::divides<>, T, Sqrt>>
    constexpr auto operator()(const vector_t<D, T, Space>& item) const -> vector_t<D, Res, Space>
    {
        const auto len = length(item);
        return len ? item / len : item;
    }
};

inline constexpr auto unit = unit_fn{};

struct distance_fn
{
    template <std::size_t D, class T, class U, class Space>
    constexpr auto operator()(const point_t<D, T, Space>& lhs, const point_t<D, U, Space>& rhs) const
        -> decltype(length(rhs - lhs))
    {
        return length(rhs - lhs);
    }
};

inline constexpr auto distance = distance_fn{};

template <side_t S>
struct get_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return item.get(S);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item) const -> point_t<D, T, Space>
    {
        return item.get(S);
    }
};

inline constexpr auto lower = get_fn<side_t::lower>{};
inline constexpr auto upper = get_fn<side_t::upper>{};

inline constexpr auto min = get_fn<side_t::first>{};
inline constexpr auto max = get_fn<side_t::last>{};

struct size_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return upper(item) - lower(item);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item) const -> extent_t<D, T, Space>
    {
        return upper(item) - lower(item);
    }
};

inline constexpr auto size = size_fn{};
inline constexpr auto extent = size;

struct radius_fn
{
    template <class T, std::size_t D, class Space>
    constexpr auto operator()(const spherical_shape_t<D, T, Space>& item) const -> T
    {
        return item.radius;
    }
};

inline constexpr auto radius = radius_fn{};

struct center_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return item.get(side_t::middle);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item) const -> point_t<D, T, Space>
    {
        return item.get(side_t::middle);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const segment_t<D, T, Space>& item) const -> point_t<D, T, Space>
    {
        return (item[0] + item[1]) / 2;
    }

    template <class T, std::size_t D, class Space>
    constexpr auto operator()(const spherical_shape_t<D, T, Space>& item) const -> point_t<D, T, Space>
    {
        return item.center;
    }
};

inline constexpr auto center = center_fn{};

struct clamp_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item, T value) const -> T
    {
        return std::min(upper(item), std::max(lower(item), value));
    }

    template <class T>
    constexpr auto operator()(const interval_t<T>& item, const interval_t<T>& value) const -> interval_t<T>
    {
        return { (*this)(item, lower(value)), (*this)(item, upper(value)) };
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item, const point_t<D, T, Space>& value) const
        -> point_t<D, T, Space>
    {
        point_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d], value[d]);
        }
        return result;
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item, const box_shape_t<D, T, Space>& value) const
        -> box_shape_t<D, T, Space>
    {
        box_shape_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d], value[d]);
        }
        return result;
    }
};

inline constexpr auto clamp = clamp_fn{};

struct extend_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item, T left, T right) const -> interval_t<T>
    {
        return { item[0] - left, item[1] + right };
    }

    template <class T>
    constexpr auto operator()(const interval_t<T>& item, T value) const -> interval_t<T>
    {
        return (*this)(item, value, value);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(
        const box_shape_t<D, T, Space>& item, const vector_t<D, T, Space>& left, const vector_t<D, T, Space>& right) const
        -> box_shape_t<D, T, Space>
    {
        box_shape_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d], left[d], right[d]);
        }
        return result;
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item, const vector_t<D, T, Space>& value) const
        -> box_shape_t<D, T, Space>
    {
        return (*this)(item, value, value);
    }
};

inline constexpr auto extend = extend_fn{};

struct orientation_fn
{
    template <class T, class U, class Space>
    constexpr auto operator()(
        const point_t<2, T, Space>& point, const point_t<2, U, Space>& start, const point_t<2, U, Space>& end) const
    {
        return cross(end - start, point - start);
    }

    template <class T, class U, class Tag, class Space>
    constexpr auto operator()(const point_t<2, T, Space>& point, const linear_shape_t<2, Tag, U, Space>& shape) const
    {
        return (*this)(point, shape[0], shape[1]);
    }
};

inline constexpr auto orientation = orientation_fn{};

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
        const T lo = lower(item);
        const T up = upper(item);
        return inclusive_between(lower(other), lo, up) && inclusive_between(upper(other), lo, up);
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item, const box_shape_t<D, T, Space>& other) const -> bool
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

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& item, const point_t<D, T, Space>& other) const -> bool
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

    template <std::size_t D, class T, class U, class Space>
    constexpr auto operator()(const spherical_shape_t<D, T, Space>& item, const point_t<D, U, Space>& other) const -> bool
    {
        return norm(other - center(item)) <= math::sqr(item.radius);
    }

    template <class T, class U, class Space>
    constexpr bool operator()(const triangle_t<2, T, Space>& item, const point_t<2, U, Space>& other) const
    {
        constexpr auto same_sign = [](int a, int b) { return (a <= 0 && b <= 0) || (a >= 0 && b >= 0); };

        int result[3] = { 0, 0, 0 };

        for (std::size_t i = 0; i < 3; ++i)
        {
            result[i] = math::sign(orientation(other, segment_t<2, T>{ item[(i + 0) % 3], item[(i + 1) % 3] }));
        }

        return same_sign(result[0], result[1]) && same_sign(result[0], result[2]) && same_sign(result[1], result[2]);
    }
};

inline constexpr auto contains = contains_fn{};

struct unite_fn
{
    template <class T, class... Tail>
    constexpr auto operator()(const interval_t<T>& head, const interval_t<T>& next, const Tail&... tail) const
        -> interval_t<T>
    {
        if constexpr (sizeof...(tail) == 0)
        {
            return { std::min(lower(head), lower(next)), std::max(upper(head), upper(next)) };
        }
        else
        {
            return (*this)((*this)(head, next), tail...);
        }
    }

    template <std::size_t D, class T, class Space, class... Tail>
    constexpr auto operator()(
        const box_shape_t<D, T, Space>& head, const box_shape_t<D, T, Space>& next, const Tail&... tail) const
        -> box_shape_t<D, T, Space>
    {
        if constexpr (sizeof...(tail) == 0)
        {
            box_shape_t<D, T, Space> result;

            for (std::size_t d = 0; d < D; ++d)
            {
                result[d] = (*this)(head[d], next[d]);
            }

            return result;
        }
        else
        {
            return (*this)((*this)(head, next), tail...);
        }
    }
};

inline constexpr auto unite = unite_fn{};

struct intersects_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& self, const interval_t<T>& other) const -> bool
    {
        const auto lo_self = lower(self);
        const auto up_self = upper(self);
        const auto lo_other = lower(other);
        const auto up_other = upper(other);

        return lo_self < up_self && lo_other < up_other && lo_self < up_other && lo_other < up_self;
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& self, const box_shape_t<D, T, Space>& other) const -> bool
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

inline constexpr auto intersects = intersects_fn{};

struct interpolate_fn
{
    template <std::size_t D, class R, class T, class Space>
    constexpr auto operator()(R r, const point_t<D, T, Space>& lhs, const point_t<D, T, Space>& rhs) const
        -> point_t<D, T, Space>
    {
        return lhs + r * (rhs - lhs);
    }

    template <std::size_t D, class R, class T, class Space>
    constexpr auto operator()(R r, const segment_t<D, T, Space>& value) const
    {
        return (*this)(r, value[0], value[1]);
    }

    template <class R, class T>
    constexpr auto operator()(R r, const interval_t<T>& item) const -> T
    {
        return static_cast<T>(lower(item) + r * size(item));
    }

    template <std::size_t D, class R, class T, class Space>
    constexpr auto operator()(const point_t<D, R, Space>& r, const box_shape_t<D, T>& item) const -> point_t<D, T, Space>
    {
        point_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(r[d], item[d]);
        }
        return result;
    }
};

inline constexpr auto interpolate = interpolate_fn{};

namespace detail
{

template <class T, class E, class Space>
constexpr auto get_line_intersection_parameter(
    const point_t<2, T, Space>& a0, const point_t<2, T, Space>& a1, const point_t<2, T, Space>& p, E epsilon) -> maybe_t<T>
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

template <class T, class E, class Space>
constexpr auto get_line_intersection_parameters(
    const point_t<2, T, Space>& a0,
    const point_t<2, T, Space>& a1,
    const point_t<2, T, Space>& b0,
    const point_t<2, T, Space>& b1,
    E epsilon) -> maybe_t<std::tuple<T, T>>
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
    template <class T>
    constexpr auto operator()(const interval_t<T>& lhs, const interval_t<T>& rhs) const -> maybe_t<interval_t<T>>
    {
        const auto lo = std::max(lower(lhs), lower(rhs));
        const auto up = std::min(upper(lhs), upper(rhs));

        if (lo < up)
        {
            return interval_t<T>{ lo, up };
        }
        return {};
    }

    template <class T, class... Tail>
    constexpr auto operator()(const interval_t<T>& head, const interval_t<T>& next, const Tail&... tail) const
        -> maybe_t<interval_t<T>>
    {
        const auto partial = (*this)(head, next);

        if constexpr (sizeof...(tail) == 0)
        {
            return partial;
        }
        else
        {
            if (partial)
            {
                return (*this)(*partial, tail...);
            }
        }
        return none;
    }

    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const box_shape_t<D, T, Space>& lhs, const box_shape_t<D, T, Space>& rhs) const
        -> maybe_t<box_shape_t<D, T, Space>>
    {
        box_shape_t<D, T, Space> result;

        for (std::size_t d = 0; d < D; ++d)
        {
            const auto interval = (*this)(lhs[d], rhs[d]);
            if (!interval)
            {
                return none;
            }

            result[d] = *interval;
        }

        return result;
    }

    template <std::size_t D, class T, class Space, class... Tail>
    constexpr auto operator()(
        const box_shape_t<D, T, Space>& head, const box_shape_t<D, T, Space>& next, const Tail&... tail) const
        -> maybe_t<box_shape_t<D, T, Space>>
    {
        const auto partial = (*this)(head, next);

        if constexpr (sizeof...(tail) == 0)
        {
            return partial;
        }
        else
        {
            if (partial)
            {
                return (*this)(*partial, tail...);
            }
        }
        return none;
    }

    template <class T, class Tag1, class Tag2, class Space, class E = T>
    constexpr auto operator()(
        const linear_shape_t<2, Tag1, T, Space>& lhs, const linear_shape_t<2, Tag2, T, Space>& rhs, E epsilon = {}) const
        -> maybe_t<point_t<2, T, Space>>
    {
        const auto par = detail::get_line_intersection_parameters(lhs[0], lhs[1], rhs[0], rhs[1], epsilon);

        if (!par)
        {
            return none;
        }

        const auto [a, b] = *par;

        if (detail::contains_param(Tag1{}, a) && detail::contains_param(Tag2{}, b))
        {
            return interpolate(a, lhs[0], lhs[1]);
        }
        return none;
    }
};

inline constexpr auto intersection = intersection_fn{};

struct projection_fn
{
    template <class T, std::size_t D, class Space>
    constexpr auto operator()(const point_t<D, T, Space>& lhs, const vector_t<D, T, Space>& rhs) const
        -> decltype(rhs * (dot(rhs, lhs) / norm(rhs)))
    {
        return rhs * (dot(rhs, lhs) / norm(rhs));
    }

    template <class T, class Tag, class Space, class E = T>
    constexpr auto operator()(
        const point_t<2, T, Space>& point, const linear_shape_t<2, Tag, T, Space>& shape, E epsilon = {}) const
        -> maybe_t<point_t<2, T, Space>>
    {
        const auto p0 = shape[0];
        const auto p1 = shape[1];

        const auto result = p0 + (*this)(point - p0, p1 - p0);

        const auto t = detail::get_line_intersection_parameter(p0, p1, result, epsilon);

        if (t && detail::contains_param(Tag{}, *t))
        {
            return result;
        }

        return none;
    }
};

inline constexpr auto projection = projection_fn{};

struct rejection_fn
{
    template <std::size_t D, class T, class Space>
    constexpr auto operator()(const point_t<D, T, Space>& lhs, const vector_t<D, T, Space>& rhs) const
        -> decltype(lhs - projection(lhs, rhs))
    {
        return lhs - projection(lhs, rhs);
    }
};

inline constexpr auto rejection = rejection_fn{};

struct perpendicular_fn
{
    template <class T, class Space>
    constexpr auto operator()(const vector_t<2, T, Space>& value) const -> vector_t<2, T, Space>
    {
        return vector_t<2, T, Space>{ -value[1], value[0] };
    }

    template <class Tag, class T, class Space>
    constexpr auto operator()(const linear_shape_t<2, Tag, T, Space>& value, const point_t<2, T, Space>& origin) const
        -> linear_shape_t<2, Tag, T, Space>
    {
        return { origin, origin + (*this)(value[1] - value[0]) };
    }

    template <class Tag, class T, class Space>
    constexpr auto operator()(const linear_shape_t<2, Tag, T, Space>& value) const -> linear_shape_t<2, Tag, T, Space>
    {
        return (*this)(value, value[0]);
    }
};

inline constexpr auto perpendicular = perpendicular_fn{};

struct altitude_fn
{
    template <typename T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& value, std::size_t index) const -> segment_t<2, T, Space>
    {
        constexpr T epsilon = T(0.1);

        const auto v = value[(index + 0) % 3];

        const auto p = projection(v, line_t<2, T>{ value[(index + 1) % 3], value[(index + 2) % 3] }, epsilon);

        return { v, *p };
    }
};

inline constexpr auto altitude = altitude_fn{};

struct centroid_fn
{
    template <typename T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& value) const -> point_t<2, T, Space>
    {
        return std::accumulate(std::begin(value), std::end(value), point_t<2, T, Space>{}) / 3;
    }
};

inline constexpr auto centroid = centroid_fn{};

struct orthocenter_fn
{
    template <typename T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& value) const -> point_t<2, T, Space>
    {
        constexpr T epsilon = T(0.0001);

        return *intersection(make_line(altitude(value, 0)), make_line(altitude(value, 1)), epsilon);
    }
};

inline constexpr auto orthocenter = orthocenter_fn{};

struct circumcenter_fn
{
    template <typename T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& value) const -> point_t<2, T, Space>
    {
        constexpr T epsilon = T(0.0001);

        const auto s0 = segment(value[0], value[1]);
        const auto s1 = segment(value[1], value[2]);

        return *intersection(make_line(perpendicular(s0, center(s0))), make_line(perpendicular(s1, center(s1))), epsilon);
    }
};

inline constexpr auto circumcenter = circumcenter_fn{};

struct incenter_fn
{
    template <typename T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& value) const -> point_t<2, T, Space>
    {
        T perimeter = T(0);
        std::array<T, 3> sides = {};
        for (std::size_t i = 0; i < 3; ++i)
        {
            sides[i] = length(segment(value[(i + 1) % 3], value[(i + 2) % 3]));
            perimeter += sides[i];
        }

        point_t<2, T, Space> result = {};

        for (std::size_t i = 0; i < 3; ++i)
        {
            result += sides[i] * value[i];
        }
        result /= perimeter;

        return result;
    }
};

inline constexpr auto incenter = incenter_fn{};

struct incircle_fn
{
    template <class T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& triangle) const -> circle_t<T, Space>
    {
        constexpr T epsilon = T(0.1);

        const auto c = incenter(triangle);
        const auto r = distance(c, *projection(c, segment(triangle[0], triangle[1]), epsilon));

        return circle_t<T, Space>{ c, static_cast<T>(r) };
    }
};

inline constexpr auto incircle = incircle_fn{};

struct circumcircle_fn
{
    template <class T, class Space>
    constexpr auto operator()(const triangle_t<2, T, Space>& triangle) const -> circle_t<T, Space>
    {
        const auto c = circumcenter(triangle);
        const auto r = distance(c, triangle[0]);

        return circle_t<T, Space>{ c, static_cast<T>(r) };
    }
};

inline constexpr auto circumcircle = circumcircle_fn{};

struct translate_fn
{
    template <class T, class U, std::size_t D, class Space, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const box_shape_t<D, T, Space>& lhs, const vector_t<D, U, Space>& offset) const
        -> box_shape_t<D, Res, Space>
    {
        return map_into(box_shape_t<D, Res, Space>{}, std::plus<>{}, lhs, offset);
    }

    template <std::size_t D, class Tag, class T, class U, class Space, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const linear_shape_t<D, Tag, T, Space>& shape, const vector_t<D, U, Space>& offset) const
        -> linear_shape_t<D, Tag, Res, Space>
    {
        return linear_shape_t<D, Tag, Res, Space>{ shape[0] + offset, shape[1] + offset };
    }

    template <class T, class U, std::size_t D, class Space>
    constexpr auto operator()(const spherical_shape_t<D, T, Space>& lhs, const vector_t<D, U, Space>& offset) const
        -> spherical_shape_t<D, T, Space>
    {
        return spherical_shape_t<D, T, Space>{ lhs.center + offset, lhs.radius };
    }

    template <
        std::size_t D,
        std::size_t N,
        class T,
        class U,
        class Space,
        class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const polygonal_shape_t<D, T, N, Space>& lhs, const vector_t<D, U, Space>& rhs) const
        -> polygonal_shape_t<D, Res, N, Space>
    {
        return map_into(polygonal_shape_t<D, Res, N, Space>{}, bind_back(std::plus<>{}, rhs), lhs);
    }

    template <std::size_t D, class T, class U, class Space, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const polygon_t<D, T, Space>& lhs, const vector_t<D, U, Space>& rhs) const
        -> polygon_t<D, Res, Space>
    {
        return map_into(polygon_t<D, Res, Space>{}, bind_back(std::plus<>{}, rhs), lhs);
    }

    template <std::size_t D, class T, class U, class Space, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const polyline_t<D, T, Space>& lhs, const vector_t<D, U, Space>& rhs) const
        -> polyline_t<D, Res, Space>
    {
        return map_into(polyline_t<D, Res, Space>{}, bind_back(std::plus<>{}, rhs), lhs);
    }
};

inline constexpr auto translate = translate_fn{};

struct transform_fn
{
    template <
        std::size_t D,
        class Tag,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        class Space,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const linear_shape_t<D, Tag, T, Space>& shape, const matrix_t<R, C, U>& transformation) const
        -> linear_shape_t<D, Tag, Res, Space>
    {
        return linear_shape_t<D, Tag, Res, Space>{ shape[0] * transformation, shape[1] * transformation };
    }

    template <
        std::size_t D,
        std::size_t N,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        class Space,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const polygonal_shape_t<D, T, N, Space>& lhs, const matrix_t<R, C, U>& rhs) const
        -> polygonal_shape_t<D, Res, N, Space>
    {
        return map_into(polygonal_shape_t<D, Res, N, Space>{}, bind_back(std::multiplies<>{}, rhs), lhs);
    }

    template <
        std::size_t D,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        class Space,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const polygon_t<D, T, Space>& lhs, const matrix_t<R, C, U>& transformation) const
        -> polygon_t<D, Res, Space>
    {
        polygon_t<D, Res, Space> result(lhs.size());
        return map_into(std::move(result), bind_back(std::multiplies<>{}, transformation), lhs);
    }

    template <
        std::size_t D,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        class Space,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const polyline_t<D, T, Space>& lhs, const matrix_t<R, C, U>& transformation) const
        -> polyline_t<D, Res, Space>
    {
        polyline_t<D, Res, Space> result(lhs.size());
        return map_into(std::move(result), bind_back(std::multiplies<>{}, transformation), lhs);
    }

    template <
        std::size_t D,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        class Space,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const box_shape_t<D, T, Space>& lhs, const matrix_t<R, C, U>& transformation) const
        -> quad_t<D, Res, Space>
    {
        return quad_t<D, Res, Space>{ lhs.get({ side_t::first, side_t::first }) * transformation,
                                      lhs.get({ side_t::last, side_t::first }) * transformation,
                                      lhs.get({ side_t::last, side_t::last }) * transformation,
                                      lhs.get({ side_t::first, side_t::last }) * transformation };
    }
};

inline constexpr auto transform = transform_fn{};

struct segments_fn
{
    template <class T, std::size_t D, std::size_t N, class Space>
    constexpr auto operator()(const polygonal_shape_t<D, T, N, Space>& value) const -> sequence_t<segment_t<D, T, Space>>
    {
        return seq::range(value.size())
            .transform([&](std::size_t i) { return segment(value[i], value[(i + 1) % value.size()]); });
    }

    template <class T, std::size_t D, class Space>
    constexpr auto operator()(const polygon_t<D, T, Space>& value) const -> sequence_t<segment_t<D, T, Space>>
    {
        return seq::range(value.size())
            .transform([&](std::size_t i) { return segment(value[i], value[(i + 1) % value.size()]); });
    }

    template <class T, std::size_t D, class Space>
    constexpr auto operator()(const polyline_t<D, T, Space>& value) const -> sequence_t<segment_t<D, T, Space>>
    {
        if (value.size() < 2)
        {
            return {};
        }
        return seq::range(static_cast<std::size_t>(value.size() - 1))
            .transform([&](std::size_t i) { return segment(value[i], value[i + 1]); });
    }

    template <class T, class Space>
    constexpr auto operator()(const box_shape_t<2, T, Space>& value) const -> sequence_t<segment_t<2, T, Space>>
    {
        return seq::range(4).transform(
            [&](std::size_t i)
            {
                switch (i)
                {
                    case 0:
                        return segment(
                            value.get({ side_t::first, side_t::first }), value.get({ side_t::last, side_t::first }));
                    case 1:
                        return segment(
                            value.get({ side_t::last, side_t::first }), value.get({ side_t::last, side_t::last }));
                    case 2:
                        return segment(
                            value.get({ side_t::last, side_t::last }), value.get({ side_t::first, side_t::last }));
                    case 3:
                        return segment(
                            value.get({ side_t::first, side_t::last }), value.get({ side_t::first, side_t::first }));
                };
                throw std::logic_error{ "Invalid segment index for box_shape_t<2, T>" };
            });
    }
};

inline constexpr auto segments = segments_fn{};

struct round_fn
{
    template <class T>
    constexpr T operator()(T value) const
    {
        return math::round(value);
    }

    template <std::size_t D, class T, class Space>
    constexpr vector_t<D, T> operator()(const vector_t<D, T, Space>& value) const
    {
        vector_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(value[d]);
        }
        return result;
    }
};

inline constexpr auto round = round_fn{};

struct floor_fn
{
    template <class T>
    constexpr T operator()(T value) const
    {
        return math::floor(value);
    }

    template <std::size_t D, class T, class Space>
    constexpr vector_t<D, T, Space> operator()(const vector_t<D, T, Space>& value) const
    {
        vector_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(value[d]);
        }
        return result;
    }
};

inline constexpr auto floor = floor_fn{};

struct ceil_fn
{
    template <class T>
    constexpr T operator()(T value) const
    {
        return math::ceil(value);
    }

    template <std::size_t D, class T, class Space>
    constexpr vector_t<D, T, Space> operator()(const vector_t<D, T, Space>& value) const
    {
        vector_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(value[d]);
        }
        return result;
    }
};

inline constexpr auto ceil = ceil_fn{};

struct fractional_part_fn
{
    template <class T>
    constexpr T operator()(T value) const
    {
        return math::fractional_part(value);
    }

    template <std::size_t D, class T, class Space>
    constexpr vector_t<D, T, Space> operator()(const vector_t<D, T, Space>& value) const
    {
        vector_t<D, T, Space> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(value[d]);
        }
        return result;
    }
};

inline constexpr auto fractional_part = fractional_part_fn{};

struct floor_and_fractional_part_fn
{
    template <class T>
    constexpr std::pair<T, T> operator()(T value) const
    {
        return math::floor_and_fractional_part(value);
    }

    template <std::size_t D, class T, class Space>
    constexpr std::pair<vector_t<D, T, Space>, vector_t<D, T, Space>> operator()(const vector_t<D, T, Space>& value) const
    {
        vector_t<D, T, Space> lo;
        vector_t<D, T, Space> frac;
        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(lo[d], frac[d]) = (*this)(value[d]);
        }
        return { lo, frac };
    }
};

inline constexpr auto floor_and_fractional_part = floor_and_fractional_part_fn{};

}  // namespace detail

using detail::altitude;
using detail::angle;
using detail::ceil;
using detail::center;
using detail::centroid;
using detail::circumcenter;
using detail::circumcircle;
using detail::clamp;
using detail::contains;
using detail::cross;
using detail::distance;
using detail::dot;
using detail::extend;
using detail::floor;
using detail::floor_and_fractional_part;
using detail::fractional_part;
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
using detail::orientation;
using detail::orthocenter;
using detail::perpendicular;
using detail::projection;
using detail::radius;
using detail::rejection;
using detail::round;
using detail::segments;
using detail::size;
using detail::transform;
using detail::translate;
using detail::unit;
using detail::unite;
using detail::upper;

}  // namespace mat

}  // namespace zx
