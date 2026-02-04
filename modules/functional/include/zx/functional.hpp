#pragma once

#include <utility>
#include <zx/pipe.hpp>

namespace zx
{

static constexpr inline struct do_all_fn
{
    template <class... Funcs>
    struct impl
    {
        std::tuple<Funcs...> m_funcs;

        template <class... Args>
        void operator()(Args&&... args) const
        {
            call(std::index_sequence_for<Funcs...>{}, std::forward<Args>(args)...);
        }

        template <class... Args, std::size_t... I>
        void call(std::index_sequence<I...>, Args&&... args) const
        {
            (std::invoke(std::get<I>(m_funcs), std::forward<Args>(args)...), ...);
        }
    };

    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const -> impl<std::decay_t<Funcs>...>
    {
        return { std::forward_as_tuple(std::forward<Funcs>(funcs)...) };
    }
} do_all;

static constexpr struct apply_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        constexpr impl(Func func) : m_func(std::move(func)) { }

        template <class T>
        T& operator()(T& item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const
    {
        return pipe(impl{ do_all(std::forward<Funcs>(funcs)...) });
    }
} apply;

static constexpr struct with_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        constexpr impl(Func func) : m_func(std::move(func)) { }

        template <class T>
        T operator()(T item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const
    {
        return pipe(impl{ do_all(std::forward<Funcs>(funcs)...) });
    }
} with;

}  // namespace zx
