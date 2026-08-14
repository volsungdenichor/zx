#pragma once

#include <cmath>
#include <functional>

namespace zx
{

namespace mat
{

namespace math
{

namespace detail
{

struct sign_fn
{
    template <class T>
    constexpr auto operator()(T v) const -> int
    {
        constexpr T zero = T{};
        if (v > zero)
        {
            return +1;
        }
        else if (v < zero)
        {
            return -1;
        }
        return 0;
    }
};

struct sqr_fn
{
    template <class T, class Res = std::invoke_result_t<std::multiplies<>, T, T>>
    constexpr auto operator()(T v) const -> Res
    {
        return v * v;
    }
};

struct sqrt_fn
{
    template <class T, class Res = decltype(std::sqrt(std::declval<T>()))>
    constexpr auto operator()(T v) const -> Res
    {
        return std::sqrt(v);
    }
};

struct abs_fn
{
    template <class T>
    constexpr auto operator()(T x) const -> T
    {
        return std::abs(x);
    }
};

struct round_fn
{
    template <class T, class Res = decltype(std::round(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::round(x);
    }
};

struct floor_fn
{
    template <class T, class Res = decltype(std::floor(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::floor(x);
    }
};

struct ceil_fn
{
    template <class T, class Res = decltype(std::ceil(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::ceil(x);
    }
};

struct fractional_part_fn
{
    template <class T>
    constexpr auto operator()(T value) const -> T
    {
        return value - std::floor(value);
    }
};

struct floor_and_fractional_part_fn
{
    template <class T>
    constexpr auto operator()(T value) const -> std::pair<T, T>
    {
        return { std::floor(value), value - std::floor(value) };
    }
};

struct sin_fn
{
    template <class T, class Res = decltype(std::sin(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::sin(x);
    }
};

struct cos_fn
{
    template <class T, class Res = decltype(std::cos(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::cos(x);
    }
};

struct atan2_fn
{
    template <class T, class Res = decltype(std::atan2(std::declval<T>(), std::declval<T>()))>
    constexpr auto operator()(T y, T x) const -> Res
    {
        return std::atan2(y, x);
    }
};

struct asin_fn
{
    template <class T, class Res = decltype(std::asin(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::asin(x);
    }
};

struct acos_fn
{
    template <class T, class Res = decltype(std::acos(std::declval<T>()))>
    constexpr auto operator()(T x) const -> Res
    {
        return std::acos(x);
    }
};

}  // namespace detail

inline constexpr auto sqr = detail::sqr_fn{};
inline constexpr auto sqrt = detail::sqrt_fn{};
inline constexpr auto abs = detail::abs_fn{};
inline constexpr auto floor = detail::floor_fn{};
inline constexpr auto ceil = detail::ceil_fn{};
inline constexpr auto fractional_part = detail::fractional_part_fn{};
inline constexpr auto sin = detail::sin_fn{};
inline constexpr auto cos = detail::cos_fn{};
inline constexpr auto atan2 = detail::atan2_fn{};
inline constexpr auto asin = detail::asin_fn{};
inline constexpr auto acos = detail::acos_fn{};
inline constexpr auto floor_and_fractional_part = detail::floor_and_fractional_part_fn{};
inline constexpr auto sign = detail::sign_fn{};
inline constexpr auto round = detail::round_fn{};

template <class T>
constexpr T pi = T{ 3.14159265358979323846 };

}  // namespace math

}  // namespace mat

}  // namespace zx