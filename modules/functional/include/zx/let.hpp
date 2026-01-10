#pragma once

#include <functional>

namespace zx
{

namespace detail
{

struct let_fn
{
    template <class Func>
    constexpr auto operator()(Func&& func) const -> std::invoke_result_t<Func>
    {
        return std::invoke(std::forward<Func>(func));
    }

    template <class T0, class Func>
    constexpr auto operator()(T0&& t0, Func&& func) const -> std::invoke_result_t<Func, T0>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0));
    }

    template <class T0, class T1, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, Func&& func) const -> std::invoke_result_t<Func, T0, T1>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0), std::forward<T1>(t1));
    }

    template <class T0, class T1, class T2, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, Func&& func) const -> std::invoke_result_t<Func, T0, T1, T2>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }

    template <class T0, class T1, class T2, class T3, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3));
    }

    template <class T0, class T1, class T2, class T3, class T4, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3, T4>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3),
            std::forward<T4>(t4));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3, T4, T5>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3),
            std::forward<T4>(t4),
            std::forward<T5>(t5));
    }
};

}  // namespace detail

static constexpr detail::let_fn let;

}  // namespace zx
