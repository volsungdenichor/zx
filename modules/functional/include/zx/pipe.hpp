#pragma once

#include <functional>
#include <tuple>

namespace zx
{

template <class... Pipes>
struct pipeline
{
    std::tuple<Pipes...> m_pipes;

    constexpr pipeline(std::tuple<Pipes...> pipes) : m_pipes{ std::move(pipes) }
    {
    }

    constexpr pipeline(Pipes... pipes) : pipeline{ std::tuple<Pipes...>{ std::move(pipes)... } }
    {
    }

private:
    template <std::size_t I, class... Args>
    constexpr auto invoke(Args&&... args) const -> decltype(std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...))
    {
        return std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) == sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(invoke<I>(std::forward<Args>(args)...))
    {
        return invoke<I>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) < sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(call<I + 1>(invoke<I>(std::forward<Args>(args)...)))
    {
        return call<I + 1>(invoke<I>(std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(call<0>(std::forward<Args>(args)...))
    {
        return call<0>(std::forward<Args>(args)...);
    }
};

template <class T>
struct is_pipeline : std::false_type
{
};

template <class... Args>
struct is_pipeline<pipeline<Args...>> : std::true_type
{
};

namespace detail
{

struct pipe_fn
{
private:
    template <class Pipe>
    constexpr auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    constexpr auto to_tuple(pipeline<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_pipes;
    }

    template <class... Pipes>
    constexpr auto from_tuple(std::tuple<Pipes...> tuple) const -> pipeline<Pipes...>
    {
        return pipeline<Pipes...>{ std::move(tuple) };
    }

public:
    template <class... Pipes>
    constexpr auto operator()(Pipes&&... pipes) const
        -> decltype(from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...));
    }
};

}  // namespace detail

static constexpr inline auto pipe = detail::pipe_fn{};

}  // namespace zx
