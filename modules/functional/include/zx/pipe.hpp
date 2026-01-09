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

private:
    template <std::size_t I, std::size_t S, class Funcs, class... Args, std::enable_if_t<(I == S - 1), int> = 0>
    static constexpr auto call_impl(const Funcs& funcs, Args&&... args) -> decltype(auto)
    {
        return std::invoke(std::get<I>(funcs), std::forward<Args>(args)...);
    }

    template <std::size_t I, std::size_t S, class Funcs, class... Args, std::enable_if_t<(I < S - 1), int> = 0>
    static constexpr auto call_impl(const Funcs& funcs, Args&&... args) -> decltype(auto)
    {
        return call_impl<I + 1, S>(funcs, std::invoke(std::get<I>(funcs), std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(auto)
    {
        return call_impl<0, sizeof...(Pipes)>(m_pipes, std::forward<Args>(args)...);
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
    static constexpr auto to_tuple(Pipe pipe) -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    static constexpr auto to_tuple(pipeline<Pipes...> pipe) -> std::tuple<Pipes...>
    {
        return pipe.m_pipes;
    }

    template <class... Pipes>
    static constexpr auto from_tuple(std::tuple<Pipes...> tuple) -> pipeline<Pipes...>
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
static constexpr inline auto fn = pipe;

}  // namespace zx
