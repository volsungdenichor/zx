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

static constexpr inline auto sqr = detail::sqr_fn{};
static constexpr inline auto sqrt = detail::sqrt_fn{};
static constexpr inline auto abs = detail::abs_fn{};
static constexpr inline auto floor = detail::floor_fn{};
static constexpr inline auto ceil = detail::ceil_fn{};
static constexpr inline auto sin = detail::sin_fn{};
static constexpr inline auto cos = detail::cos_fn{};
static constexpr inline auto atan2 = detail::atan2_fn{};
static constexpr inline auto asin = detail::asin_fn{};
static constexpr inline auto acos = detail::acos_fn{};
static constexpr inline auto sign = detail::sign_fn{};

template <class T>
constexpr T pi = T{ 3.14159265358979323846 };

}  // namespace math

}  // namespace mat

}  // namespace zx