#pragma once

#include <functional>
#include <zx/maybe.hpp>
#include <zx/result.hpp>
#include <zx/type_traits.hpp>

namespace zx
{

struct fold_left_fn
{
    template <
        class Iter,
        class T,
        class Func,
        class Res = std::decay_t<std::invoke_result_t<Func, T, iter_reference_t<Iter>>>>
    auto operator()(Iter begin, Iter end, T init, Func func) const -> Res
    {
        auto state = Res{ std::move(init) };
        for (auto it = begin; it != end; ++it)
        {
            state = std::invoke(func, std::move(state), *it);
        }
        return state;
    }

    template <class Range, class T, class Func>
    auto operator()(Range&& range, T init, Func&& func) const
        -> decltype((*this)(std::begin(range), std::end(range), std::move(init), std::forward<Func>(func)))
    {
        return (*this)(std::begin(range), std::end(range), std::move(init), std::forward<Func>(func));
    }
};

static constexpr inline auto fold_left = fold_left_fn{};

struct fold_left_first_fn
{
    template <class Iter, class Func>
    auto operator()(Iter begin, Iter end, Func&& func) const
    {
        using result_type = decltype(fold_left(begin, end, *begin, std::forward<Func>(func)));

        if (begin == end)
        {
            return maybe_t<result_type>{};
        }
        result_type init = result_type{ *begin };
        return maybe_t<result_type>{ fold_left(std::next(begin), end, std::move(init), std::forward<Func>(func)) };
    }

    template <class Range, class Func>
    auto operator()(Range&& range, Func&& func) const
    {
        return (*this)(std::begin(range), std::end(range), std::forward<Func>(func));
    }
};

static constexpr inline auto fold_left_first = fold_left_first_fn{};

struct try_fold_left_fn
{
    template <
        class Iter,
        class T,
        class Func,
        class Res = std::decay_t<std::invoke_result_t<Func, T, iter_reference_t<Iter>>>>
    auto operator()(Iter begin, Iter end, T init, Func func) const -> Res
    {
        auto state = typename Res::value_type{ std::move(init) };
        for (auto it = begin; it != end; ++it)
        {
            auto result = std::invoke(func, std::move(state), *it);
            if (result.has_error())
            {
                return zx::error(std::move(result).error());
            }
            state = *std::move(result);
        }
        return state;
    }

    template <class Range, class T, class Func>
    auto operator()(Range&& range, T init, Func&& func) const
    {
        return (*this)(std::begin(range), std::end(range), std::move(init), std::forward<Func>(func));
    }
};

static constexpr inline auto try_fold_left = try_fold_left_fn{};

struct try_fold_left_first_fn
{
    template <
        class Iter,
        class Func,
        class Res = std::decay_t<std::invoke_result_t<Func, iter_reference_t<Iter>, iter_reference_t<Iter>>>,
        class V = typename Res::value_type,
        class E = typename Res::error_type>
    auto operator()(Iter begin, Iter end, Func func) const -> result_t<maybe_t<V>, E>
    {
        if (begin == end)
        {
            return maybe_t<V>{};
        }
        auto state = *begin;
        return try_fold_left(std::next(begin), end, std::move(state), std::forward<Func>(func))
            .transform([](auto&& value) { return maybe_t<V>{ std::forward<decltype(value)>(value) }; });
    }

    template <class Range, class Func>
    auto operator()(Range&& range, Func&& func) const
    {
        return (*this)(std::begin(range), std::end(range), std::forward<Func>(func));
    }
};

static constexpr inline auto try_fold_left_first = try_fold_left_first_fn{};

}  // namespace zx
