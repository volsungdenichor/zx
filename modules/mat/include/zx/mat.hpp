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

template <std::size_t D, class T>
constexpr line_t<2, T> make_line(const segment_t<D, T>& s)
{
    return line_t<2, T>{ s[0], s[1] };
}

struct dot_fn
{
    template <std::size_t D, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<D, T>& lhs, const vector_t<D, U>& rhs) const -> Res
    {
        return std::inner_product(std::begin(lhs), std::end(lhs), std::begin(rhs), Res{});
    }
};

static constexpr inline auto dot = dot_fn{};

struct cross_fn
{
    template <class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<2, T>& lhs, const vector_t<2, U>& rhs) const -> Res
    {
        return lhs[0] * rhs[1] - lhs[1] * rhs[0];
    }

    template <class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const vector_t<3, T>& lhs, const vector_t<3, U>& rhs) const -> vector_t<3, Res>
    {
        return vector_t<3, Res>{ { lhs[1] * rhs[2] - lhs[2] * rhs[1],  //
                                   lhs[2] * rhs[0] - lhs[0] * rhs[2],
                                   lhs[0] * rhs[1] - lhs[1] * rhs[0] } };
    }
};

static constexpr inline auto cross = cross_fn{};

struct angle_fn
{
    template <class T>
    constexpr auto operator()(const vector_t<2, T>& lhs, const vector_t<2, T>& rhs) const
        -> decltype(math::atan2(cross(lhs, rhs), dot(lhs, rhs)))
    {
        return math::atan2(cross(lhs, rhs), dot(lhs, rhs));
    }

    template <class T>
    constexpr auto operator()(const vector_t<3, T>& lhs, const vector_t<3, T>& rhs) const
        -> decltype(math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs))))
    {
        return math::acos(dot(lhs, rhs) / (length(lhs) * length(rhs)));
    }
};

static constexpr inline auto angle = angle_fn{};

struct norm_fn
{
    template <std::size_t D, class T, class Res = std::invoke_result_t<std::multiplies<>, T, T>>
    constexpr auto operator()(const vector_t<D, T>& item) const -> Res
    {
        return dot(item, item);
    }
};

static constexpr inline auto norm = norm_fn{};

struct length_fn
{
    template <std::size_t D, class T>
    constexpr auto operator()(const vector_t<D, T>& item) const -> decltype(math::sqrt(norm(item)))
    {
        return math::sqrt(norm(item));
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const segment_t<D, T>& item) const
    {
        return (*this)(item[1] - item[0]);
    }
};

static constexpr inline auto length = length_fn{};

struct unit_fn
{
    template <
        std::size_t D,
        class T,
        class Sqr = std::invoke_result_t<std::multiplies<>, T, T>,
        class Sqrt = decltype(sqrt(std::declval<Sqr>())),
        class Res = std::invoke_result_t<std::divides<>, T, Sqrt>>
    constexpr auto operator()(const vector_t<D, T>& item) const -> vector_t<D, Res>
    {
        const auto len = length(item);
        return len ? item / len : item;
    }
};

static constexpr inline auto unit = unit_fn{};

struct distance_fn
{
    template <std::size_t D, class T, class U>
    constexpr auto operator()(const point_t<D, T>& lhs, const point_t<D, U>& rhs) const -> decltype(length(rhs - lhs))
    {
        return length(rhs - lhs);
    }
};

static constexpr inline auto distance = distance_fn{};

template <side_t S>
struct get_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return item.get(S);
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item) const -> point_t<D, T>
    {
        return item.get(S);
    }
};

static constexpr inline auto lower = get_fn<side_t::lower>{};
static constexpr inline auto upper = get_fn<side_t::upper>{};

static constexpr inline auto min = get_fn<side_t::first>{};
static constexpr inline auto max = get_fn<side_t::last>{};

struct size_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return upper(item) - lower(item);
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item) const -> extent_t<D, T>
    {
        return upper(item) - lower(item);
    }
};

static constexpr inline auto size = size_fn{};

struct radius_fn
{
    template <class T, std::size_t D>
    constexpr auto operator()(const spherical_shape_t<D, T>& item) const -> T
    {
        return item.radius;
    }
};

static constexpr inline auto radius = radius_fn{};

struct center_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item) const -> T
    {
        return item.get(side_t::middle);
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item) const -> point_t<D, T>
    {
        return item.get(side_t::middle);
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const segment_t<D, T>& item) const -> point_t<D, T>
    {
        return (item[0] + item[1]) / 2;
    }

    template <class T, std::size_t D>
    constexpr auto operator()(const spherical_shape_t<D, T>& item) const -> point_t<D, T>
    {
        return item.center;
    }
};

static constexpr inline auto center = center_fn{};

struct extend_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& item, T value) const -> interval_t<T>
    {
        return { item[0] - value, item[1] + value };
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item, const vector_t<D, T>& value) const -> box_shape_t<D, T>
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d], value[d]);
        }
        return result;
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item, T value) const -> box_shape_t<D, T>
    {
        box_shape_t<D, T> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(item[d], value);
        }
        return result;
    }
};

static constexpr inline auto extend = extend_fn{};

struct orientation_fn
{
    template <class T, class U>
    constexpr auto operator()(const point_t<2, T>& point, const point_t<2, U>& start, const point_t<2, U>& end) const
    {
        return cross(end - start, point - start);
    }

    template <class T, class U, class Tag>
    constexpr auto operator()(const point_t<2, T>& point, const linear_shape_t<2, Tag, U>& shape) const
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
        const T lo = lower(item);
        const T up = upper(item);
        return inclusive_between(lower(other), lo, up) && inclusive_between(upper(other), lo, up);
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item, const box_shape_t<D, T>& other) const -> bool
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

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& item, const point_t<D, T>& other) const -> bool
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

    template <std::size_t D, class T, class U>
    constexpr auto operator()(const spherical_shape_t<D, T>& item, const point_t<D, U>& other) const -> bool
    {
        return norm(other - center(item)) <= math::sqr(item.radius);
    }

    template <class T, class U>
    constexpr bool operator()(const triangle_t<2, T>& item, const point_t<2, U>& other) const
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

static constexpr inline auto contains = contains_fn{};

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

    template <std::size_t D, class T, class... Tail>
    constexpr auto operator()(const box_shape_t<D, T>& head, const box_shape_t<D, T>& next, const Tail&... tail) const
        -> box_shape_t<D, T>
    {
        if constexpr (sizeof...(tail) == 0)
        {
            box_shape_t<D, T> result;

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

static constexpr inline auto unite = unite_fn{};

struct intersects_fn
{
    template <class T>
    constexpr auto operator()(const interval_t<T>& self, const interval_t<T>& other) const -> bool
    {
        const auto lo_self = lower(self);
        const auto up_self = upper(self);
        const auto lo_other = lower(other);
        const auto up_other = upper(other);

        // Intervals are represented as [lo, up), so touching endpoints do not intersect.
        return lo_self < up_self && lo_other < up_other && lo_self < up_other && lo_other < up_self;
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& self, const box_shape_t<D, T>& other) const -> bool
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
    template <std::size_t D, class R, class T>
    constexpr auto operator()(R r, const point_t<D, T>& lhs, const point_t<D, T>& rhs) const -> point_t<D, T>
    {
        return lhs + r * (rhs - lhs);
    }

    template <std::size_t D, class R, class T>
    constexpr auto operator()(R r, const segment_t<D, T>& value) const
    {
        return (*this)(r, value[0], value[1]);
    }

    template <class R, class T>
    constexpr auto operator()(R r, const interval_t<T>& item) const
    {
        return lower(item) + r * size(item);
    }
};

static constexpr inline auto interpolate = interpolate_fn{};

namespace detail
{

template <class T, class E>
constexpr auto get_line_intersection_parameter(
    const point_t<2, T>& a0, const point_t<2, T>& a1, const point_t<2, T>& p, E epsilon) -> std::optional<T>
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
    const point_t<2, T>& a0, const point_t<2, T>& a1, const point_t<2, T>& b0, const point_t<2, T>& b1, E epsilon)
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
    template <class T>
    constexpr auto operator()(const interval_t<T>& lhs, const interval_t<T>& rhs) const -> std::optional<interval_t<T>>
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
        -> std::optional<interval_t<T>>
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
        return std::nullopt;
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const box_shape_t<D, T>& lhs, const box_shape_t<D, T>& rhs) const
        -> std::optional<box_shape_t<D, T>>
    {
        box_shape_t<D, T> result;

        for (std::size_t d = 0; d < D; ++d)
        {
            const auto interval = (*this)(lhs[d], rhs[d]);
            if (!interval)
            {
                return {};
            }

            result[d] = *interval;
        }

        return result;
    }

    template <std::size_t D, class T, class... Tail>
    constexpr auto operator()(const box_shape_t<D, T>& head, const box_shape_t<D, T>& next, const Tail&... tail) const
        -> std::optional<box_shape_t<D, T>>
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
        return std::nullopt;
    }

    template <class T, class Tag1, class Tag2, class E = T>
    constexpr auto operator()(const linear_shape_t<2, Tag1, T>& lhs, const linear_shape_t<2, Tag2, T>& rhs, E epsilon = {})
        const -> std::optional<point_t<2, T>>
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
    constexpr auto operator()(const point_t<D, T>& lhs, const vector_t<D, T>& rhs) const
        -> decltype(rhs * (dot(rhs, lhs) / norm(rhs)))
    {
        return rhs * (dot(rhs, lhs) / norm(rhs));
    }

    template <class T, class Tag, class E = T>
    constexpr auto operator()(const point_t<2, T>& point, const linear_shape_t<2, Tag, T>& shape, E epsilon = {}) const
        -> std::optional<point_t<2, T>>
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
    template <std::size_t D, class T>
    constexpr auto operator()(const point_t<D, T>& lhs, const vector_t<D, T>& rhs) const
        -> decltype(lhs - projection(lhs, rhs))
    {
        return lhs - projection(lhs, rhs);
    }
};

static constexpr inline auto rejection = rejection_fn{};

struct perpendicular_fn
{
    template <class T>
    constexpr auto operator()(const vector_t<2, T>& value) const -> vector_t<2, T>
    {
        return vector_t<2, T>{ -value[1], value[0] };
    }

    template <class Tag, class T>
    constexpr auto operator()(const linear_shape_t<2, Tag, T>& value, const point_t<2, T>& origin) const
        -> linear_shape_t<2, Tag, T>
    {
        return { origin, origin + (*this)(value[1] - value[0]) };
    }

    template <class Tag, class T>
    constexpr auto operator()(const linear_shape_t<2, Tag, T>& value) const -> linear_shape_t<2, Tag, T>
    {
        return (*this)(value, value[0]);
    }
};

static constexpr inline auto perpendicular = perpendicular_fn{};

struct altitude_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<2, T>& value, std::size_t index) const -> segment_t<2, T>
    {
        constexpr T epsilon = T(0.1);

        const auto v = value[(index + 0) % 3];

        const auto p = projection(v, line_t<2, T>{ value[(index + 1) % 3], value[(index + 2) % 3] }, epsilon);

        return { v, *p };
    }
};

static constexpr inline auto altitude = altitude_fn{};

struct centroid_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<2, T>& value) const -> point_t<2, T>
    {
        return std::accumulate(std::begin(value), std::end(value), point_t<2, T>{}) / 3;
    }
};

static constexpr inline auto centroid = centroid_fn{};

struct orthocenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<2, T>& value) const -> point_t<2, T>
    {
        constexpr T epsilon = T(0.0001);

        return *intersection(make_line(altitude(value, 0)), make_line(altitude(value, 1)), epsilon);
    }
};

static constexpr inline auto orthocenter = orthocenter_fn{};

struct circumcenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<2, T>& value) const -> point_t<2, T>
    {
        constexpr T epsilon = T(0.0001);

        const auto s0 = segment_t<2, T>{ value[0], value[1] };
        const auto s1 = segment_t<2, T>{ value[1], value[2] };

        return *intersection(make_line(perpendicular(s0, center(s0))), make_line(perpendicular(s1, center(s1))), epsilon);
    }
};

static constexpr inline auto circumcenter = circumcenter_fn{};

struct incenter_fn
{
    template <typename T>
    constexpr auto operator()(const triangle_t<2, T>& value) const -> point_t<2, T>
    {
        T perimeter = T(0);
        std::array<T, 3> sides = {};
        for (std::size_t i = 0; i < 3; ++i)
        {
            sides[i] = length(segment_t<2, T>{ value[(i + 1) % 3], value[(i + 2) % 3] });
            perimeter += sides[i];
        }

        point_t<2, T> result = {};

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
    constexpr auto operator()(const triangle_t<2, T>& triangle_t) const -> spherical_shape_t<2, T>
    {
        constexpr T epsilon = T(0.1);

        const auto c = incenter(triangle_t);
        const auto r = distance(c, *projection(c, segment_t<2, T>{ triangle_t[0], triangle_t[1] }, epsilon));

        return spherical_shape_t<2, T>{ c, static_cast<T>(r) };
    }
};

static constexpr inline auto incircle = incircle_fn{};

struct circumcircle_fn
{
    template <class T>
    constexpr auto operator()(const triangle_t<2, T>& triangle_t) const -> spherical_shape_t<2, T>
    {
        const auto c = circumcenter(triangle_t);
        const auto r = distance(c, triangle_t[0]);

        return spherical_shape_t<2, T>{ c, static_cast<T>(r) };
    }
};

static constexpr inline auto circumcircle = circumcircle_fn{};

struct translate_fn
{
    template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const box_shape_t<D, T>& lhs, const vector_t<D, U>& offset) const -> box_shape_t<D, Res>
    {
        return map_into(box_shape_t<D, Res>{}, std::plus<>{}, lhs, offset);
    }

    template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const linear_shape_t<D, Tag, T>& shape, const vector_t<D, U>& offset) const
        -> linear_shape_t<D, Tag, Res>
    {
        return linear_shape_t<D, Tag, Res>{ shape[0] + offset, shape[1] + offset };
    }

    template <std::size_t D, class Tag, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const vertex_list_shape_t<D, Tag, T>& lhs, const vector_t<D, U>& offset) const
        -> vertex_list_shape_t<D, Tag, Res>
    {
        return map_into(
            vertex_list_shape_t<D, Tag, Res>{}, bind_back(std::plus<>{}, offset), lhs);
    }

    template <class T, class U, std::size_t D>
    constexpr auto operator()(spherical_shape_t<D, T> lhs, const vector_t<D, U>& offset) const -> spherical_shape_t<D, T>
    {
        return spherical_shape_t<D, T>{ lhs.center + offset, lhs.radius };
    }

    template <std::size_t D, std::size_t N, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
    constexpr auto operator()(const polygonal_shape_t<D, T, N>& lhs, const vector_t<D, U>& rhs) const
        -> polygonal_shape_t<D, Res, N>
    {
        return map_into(polygonal_shape_t<D, Res, N>{}, bind_back(std::plus<>{}, rhs), lhs);
    }
};

static constexpr inline auto translate = translate_fn{};

struct transform_fn
{
    template <
        std::size_t D,
        class Tag,
        class T,
        std::size_t R,
        std::size_t C,
        class U,
        enable_if_t<(R == D + 1 && C == D + 1)> = 0,
        class Res = std::invoke_result_t<std::multiplies<>, T, U>>
    constexpr auto operator()(const linear_shape_t<D, Tag, T>& shape, const matrix_t<R, C, U>& transformation) const
        -> linear_shape_t<D, Tag, Res>
    {
        return linear_shape_t<D, Tag, Res>{ shape[0] * transformation, shape[1] * transformation };
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
    constexpr auto operator()(const polygonal_shape_t<D, T, N>& lhs, const matrix_t<R, C, U>& rhs) const
        -> polygonal_shape_t<D, Res, N>
    {
        return map_into(polygonal_shape_t<D, Res, N>{}, bind_back(std::multiplies<>{}, rhs), lhs);
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
    constexpr auto operator()(const vertex_list_shape_t<D, Tag, T>& lhs, const matrix_t<R, C, U>& transformation) const
        -> vertex_list_shape_t<D, Tag, Res>
    {
        vertex_list_shape_t<D, Tag, Res> result(lhs.size());
        return map_into(std::move(result), bind_back(std::multiplies<>{}, transformation), lhs);
    }
};

static constexpr inline auto transform = transform_fn{};

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
using detail::extend;
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
using detail::size;
using detail::transform;
using detail::translate;
using detail::unit;
using detail::unite;
using detail::upper;

}  // namespace mat

}  // namespace zx
