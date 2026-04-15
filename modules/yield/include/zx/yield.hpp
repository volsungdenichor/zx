#pragma once

#include <bitset>
#include <functional>
#include <tuple>
#include <type_traits>
#include <zx/type_traits.hpp>

namespace zx
{

enum class step_t
{
    loop_continue,
    loop_break,
};

namespace detail
{

template <class T, class R>
using is_transducer_impl = decltype(std::declval<T>().transduce(std::declval<R>()));

template <class T, class R>
struct is_transducer : is_detected<is_transducer_impl, T, R>
{
};

template <class T, class R>
using is_generator_impl = decltype(std::declval<T>().yield_to(std::declval<R>()));

template <class T, class R>
struct is_generator : is_detected<is_generator_impl, T, R>
{
};

}  // namespace detail

template <class State, class Reducer>
struct reductor_t
{
    using state_type = State;
    using reducer_type = Reducer;

    state_type state;
    reducer_type reducer;

    template <class... Args>
    step_t operator()(Args&&... args)
    {
        return reducer.reduce(state, std::forward<Args>(args)...);
    }

    template <class Transducer, std::enable_if_t<detail::is_transducer<Transducer, reducer_type>::value, int> = 0>
    friend constexpr auto operator|=(Transducer&& transducer, const reductor_t& reductor)
    {
        return reductor_t<state_type, decltype(transducer.transduce(reductor.reducer))>{
            reductor.state, transducer.transduce(reductor.reducer)
        };
    }

    template <class Transducer, std::enable_if_t<detail::is_transducer<Transducer, reducer_type>::value, int> = 0>
    friend constexpr auto operator|=(Transducer&& transducer, reductor_t&& reductor)
    {
        return reductor_t<state_type, decltype(transducer.transduce(std::move(reductor.reducer)))>{
            std::move(reductor.state), transducer.transduce(std::move(reductor.reducer))
        };
    }

    template <class Generator, std::enable_if_t<detail::is_generator<Generator, reductor_t>::value, int> = 0>
    friend constexpr auto operator|=(Generator&& generator, const reductor_t& reductor) -> state_type
    {
        return generator.yield_to(reductor);
    }

    template <class Generator, std::enable_if_t<detail::is_generator<Generator, reductor_t>::value, int> = 0>
    friend constexpr auto operator|=(Generator&& generator, reductor_t&& reductor) -> state_type
    {
        return generator.yield_to(std::move(reductor));
    }
};

template <class State, class Reducer>
reductor_t(State&&, Reducer&&) -> reductor_t<std::decay_t<State>, std::decay_t<Reducer>>;

namespace detail
{
template <class T>
struct is_reductor : std::false_type
{
};

template <class State, class Reducer>
struct is_reductor<reductor_t<State, Reducer>> : std::true_type
{
};

template <class T>
struct state_type;

template <class T>
using state_type_t = typename state_type<T>::type;

template <class State, class Reducer>
struct state_type<reductor_t<State, Reducer>>
{
    using type = State;
};

}  // namespace detail

struct combine_fn
{
    template <class... Transducers>
    struct transducer_t
    {
        std::tuple<Transducers...> m_transducers;

        template <std::size_t N, class NextReducer>
        constexpr auto apply_adapters(NextReducer&& next_reducer) const
        {
            if constexpr (N == 0)
            {
                return std::get<N>(m_transducers).transduce(std::forward<NextReducer>(next_reducer));
            }
            else
            {
                return apply_adapters<N - 1>(std::get<N>(m_transducers).transduce(std::forward<NextReducer>(next_reducer)));
            }
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            if constexpr (sizeof...(Transducers) == 0)
            {
                return std::forward<NextReducer>(next_reducer);
            }
            else
            {
                return apply_adapters<sizeof...(Transducers) - 1>(std::forward<NextReducer>(next_reducer));
            }
        }

        template <class State, class Reducer>
        constexpr auto to_reductor(reductor_t<State, Reducer> reductor) const
        {
            return reductor_t{ std::move(reductor.state), transduce(reductor.reducer) };
        }
    };

    template <class... Transducers>
    constexpr auto operator()(Transducers&&... transducers) const
    {
        return transducer_t<std::decay_t<Transducers>...>{ { std::forward<Transducers>(transducers)... } };
    }
} combine = combine_fn{};

struct transduce_fn
{
    template <std::size_t Last, class Tuple, std::size_t... I>
    static constexpr auto impl(Tuple&& tuple, std::index_sequence<I...>)
    {
        return combine(std::get<I>(std::forward<Tuple>(tuple))...).to_reductor(std::get<Last>(std::forward<Tuple>(tuple)));
    }

    template <class... Args>
    constexpr auto operator()(Args&&... args) const
    {
        constexpr std::size_t num_args = sizeof...(Args);
        static_assert(num_args >= 1, "transduce requires at least a reductor argument");
        constexpr std::size_t last = num_args - 1;
        return impl<last>(std::forward_as_tuple(std::forward<Args>(args)...), std::make_index_sequence<last>{});
    }
} transduce = transduce_fn{};

namespace generators
{

template <class Impl>
struct generator_t
{
    Impl m_impl;

    template <class Reductor, std::enable_if_t<detail::is_reductor<std::decay_t<Reductor>>::value, int> = 0>
    auto yield_to(Reductor&& reductor) const -> detail::state_type_t<std::decay_t<Reductor>>
    {
        std::invoke(m_impl, std::forward<Reductor>(reductor));
        return reductor.state;
    }
};

template <class Impl>
generator_t(Impl&&) -> generator_t<std::decay_t<Impl>>;

template <class Impl>
constexpr auto generate(Impl&& impl) -> generator_t<std::decay_t<Impl>>
{
    return generator_t<std::decay_t<Impl>>{ std::forward<Impl>(impl) };
}

namespace detail
{

struct range_fn
{
    template <class T>
    struct generator_t
    {
        T m_lower;
        T m_upper;

        constexpr generator_t(T lower, T upper) : m_lower(lower), m_upper(upper) { }

        template <class Reductor>
        void operator()(Reductor&& reductor) const
        {
            for (T it = m_lower; it < m_upper; ++it)
            {
                if (reductor(it) == step_t::loop_break)
                {
                    return;
                }
            }
        }
    };

    template <class T>
    constexpr auto operator()(T lower, T upper) const
    {
        return generate(generator_t<T>{ lower, upper });
    }

    template <class T>
    constexpr auto operator()(T upper) const
    {
        return (*this)(T{}, upper);
    }
};

struct iota_fn
{
    template <class T>
    struct generator_t
    {
        T m_lower;

        template <class Reductor>
        void operator()(Reductor&& reductor) const
        {
            for (T it = m_lower;; ++it)
            {
                if (reductor(it) == step_t::loop_break)
                {
                    return;
                }
            }
        }
    };

    template <class T = std::ptrdiff_t>
    constexpr auto operator()(T lower = {}) const
    {
        return generate(generator_t<T>{ lower });
    }
};

struct from_fn
{
    template <class It>
    struct generator_t
    {
        It m_begin;
        It m_end;

        template <class Reductor>
        void operator()(Reductor&& reductor) const
        {
            for (It it = m_begin; it != m_end; ++it)
            {
                if (reductor(*it) == step_t::loop_break)
                {
                    return;
                }
            }
        }
    };

    template <class Range>
    constexpr auto operator()(Range&& range) const
    {
        return generate(generator_t<decltype(std::begin(range))>{ std::begin(range), std::end(range) });
    }
};

struct chain_fn
{
    template <class... Generators>
    struct generator_t
    {
        std::tuple<Generators...> m_generators;

        template <std::size_t N, class Reductor>
        void handle(Reductor&& reductor) const
        {
            if constexpr (N == sizeof...(Generators))
            {
                return;
            }
            else
            {
                std::get<N>(m_generators).yield_to(std::forward<Reductor>(reductor));
                handle<N + 1>(std::forward<Reductor>(reductor));
            }
        }

        template <class Reductor>
        void operator()(Reductor&& reductor) const
        {
            handle<0>(std::forward<Reductor>(reductor));
        }
    };

    template <class... Generators>
    constexpr auto operator()(Generators&&... generators) const
    {
        return generate(generator_t<std::decay_t<Generators>...>{ { std::forward<Generators>(generators)... } });
    }
};

struct repeat_fn
{
    template <class... Args>
    struct generator_t
    {
        std::tuple<Args...> m_args;

        template <class Reductor>
        void operator()(Reductor&& reductor) const
        {
            bool done = false;
            while (!done)
            {
                std::apply(
                    [&](const auto&... args)
                    {
                        if (reductor(args...) == step_t::loop_break)
                        {
                            done = true;
                            return;
                        }
                    },
                    m_args);
            }
        }
    };

    template <class... Args>
    constexpr auto operator()(Args&&... args) const
    {
        return generate(generator_t<std::decay_t<Args>...>{ { std::forward<Args>(args)... } });
    }
};

}  // namespace detail

static constexpr inline auto range = detail::range_fn{};
static constexpr inline auto iota = detail::iota_fn{};
static constexpr inline auto from = detail::from_fn{};
static constexpr inline auto chain = detail::chain_fn{};
static constexpr inline auto repeat = detail::repeat_fn{};

}  // namespace generators

namespace transducers
{

namespace detail
{

template <bool Indexed>
struct transform_fn
{
    template <bool Indexed_, class Func, class NextReducer>
    struct reducer_t;

    template <class Func, class NextReducer>
    struct reducer_t<false, Func, NextReducer>
    {
        Func m_func;
        NextReducer m_next_reducer;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return m_next_reducer.reduce(state, std::invoke(m_func, std::forward<Args>(args)...));
        }
    };

    template <class Func, class NextReducer>
    struct reducer_t<true, Func, NextReducer>
    {
        Func m_func;
        NextReducer m_next_reducer;
        mutable std::size_t m_index = 0;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return m_next_reducer.reduce(state, std::invoke(m_func, m_index++, std::forward<Args>(args)...));
        }
    };

    template <class Func>
    struct transducer_t
    {
        Func m_func;

        template <class Reducer>
        constexpr auto transduce(Reducer&& reducer) const&
        {
            return reducer_t<Indexed, Func, std::decay_t<Reducer>>{ m_func, std::forward<Reducer>(reducer) };
        }

        template <class Reducer>
        constexpr auto transduce(Reducer&& reducer) &&
        {
            return reducer_t<Indexed, Func, std::decay_t<Reducer>>{ std::move(m_func), std::forward<Reducer>(reducer) };
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return transducer_t<std::decay_t<Func>>{ std::forward<Func>(func) };
    }
};

template <bool Indexed>
struct filter_fn
{
    template <bool Indexed_, class Pred, class NextReducer>
    struct reducer_t;

    template <class Pred, class NextReducer>
    struct reducer_t<false, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return std::invoke(m_pred, std::forward<Args>(args)...)
                       ? m_next_reducer.reduce(state, std::forward<Args>(args)...)
                       : step_t::loop_continue;
        }
    };

    template <class Pred, class NextReducer>
    struct reducer_t<true, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable std::size_t m_index = 0;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return std::invoke(m_pred, m_index++, std::forward<Args>(args)...)
                       ? m_next_reducer.reduce(state, std::forward<Args>(args)...)
                       : step_t::loop_continue;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ std::move(m_pred),
                                                                        std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return transducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

template <bool Indexed>
struct take_while_fn
{
    template <bool Indexed_, class Pred, class NextReducer>
    struct reducer_t;

    template <class Pred, class NextReducer>
    struct reducer_t<false, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, std::forward<Args>(args)...);
            return !m_done ? m_next_reducer.reduce(state, std::forward<Args>(args)...) : step_t::loop_continue;
        }
    };

    template <class Pred, class NextReducer>
    struct reducer_t<true, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;
        mutable std::size_t m_index = 0;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, m_index++, std::forward<Args>(args)...);
            return !m_done ? m_next_reducer.reduce(state, std::forward<Args>(args)...) : step_t::loop_continue;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ std::move(m_pred),
                                                                        std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return transducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

template <bool Indexed>
struct drop_while_fn
{
    template <bool Indexed_, class Pred, class NextReducer>
    struct reducer_t;

    template <class Pred, class NextReducer>
    struct reducer_t<false, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, std::forward<Args>(args)...);
            return m_done ? m_next_reducer.reduce(state, std::forward<Args>(args)...) : step_t::loop_continue;
        }
    };

    template <class Pred, class NextReducer>
    struct reducer_t<true, Pred, NextReducer>
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;
        mutable std::size_t m_index = 0;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, m_index++, std::forward<Args>(args)...);
            return m_done ? m_next_reducer.reduce(state, std::forward<Args>(args)...) : step_t::loop_continue;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Indexed, Pred, std::decay_t<NextReducer>>{ std::move(m_pred),
                                                                        std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return transducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct take_fn
{
    template <class NextReducer>
    struct reducer_t
    {
        mutable std::ptrdiff_t m_count;
        NextReducer m_next_reducer;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            if (m_count-- > 0)
            {
                return m_next_reducer.reduce(state, std::forward<Args>(args)...);
            }
            return step_t::loop_break;
        }
    };

    struct transducer_t
    {
        std::ptrdiff_t m_count;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            return reducer_t<std::decay_t<NextReducer>>{ m_count, std::forward<NextReducer>(next_reducer) };
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const { return transducer_t{ count }; }
};

struct drop_fn
{
    template <class NextReducer>
    struct reducer_t
    {
        mutable std::ptrdiff_t m_count;
        NextReducer m_next_reducer;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            if (m_count-- > 0)
            {
                return step_t::loop_continue;
            }
            return m_next_reducer.reduce(state, std::forward<Args>(args)...);
        }
    };

    struct transducer_t
    {
        std::ptrdiff_t m_count;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            return reducer_t<std::decay_t<NextReducer>>{ m_count, std::forward<NextReducer>(next_reducer) };
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const { return transducer_t{ count }; }
};

struct join_fn
{
    template <class NextReducer>
    struct reducer_t
    {
        NextReducer m_next_reducer;

        template <class State, class Arg>
        step_t reduce(State& state, Arg&& arg) const
        {
            for (auto&& item : arg)
            {
                if (m_next_reducer.reduce(state, std::forward<decltype(item)>(item)) == step_t::loop_break)
                {
                    return step_t::loop_break;
                }
            }
            return step_t::loop_continue;
        }
    };

    struct transducer_t
    {
        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            return reducer_t<std::decay_t<NextReducer>>{ std::forward<NextReducer>(next_reducer) };
        }
    };

    constexpr auto operator()() const { return transducer_t{}; }
};

struct intersperse_fn
{
    template <class Separator, class NextReducer>
    struct reducer_t
    {
        Separator m_separator;
        NextReducer m_next_reducer;
        mutable bool m_first = true;

        template <class State, class Arg>
        step_t reduce(State& state, Arg&& arg) const
        {
            if (!m_first)
            {
                if (m_next_reducer.reduce(state, m_separator) == step_t::loop_break)
                {
                    return step_t::loop_break;
                }
            }
            m_first = false;
            return m_next_reducer.reduce(state, std::forward<Arg>(arg));
        }
    };

    template <class Separator>
    struct transducer_t
    {
        Separator m_separator;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Separator, std::decay_t<NextReducer>>{ m_separator, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Separator, std::decay_t<NextReducer>>{ std::move(m_separator),
                                                                    std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Separator>
    constexpr auto operator()(Separator&& separator) const
    {
        return transducer_t<std::decay_t<Separator>>{ std::forward<Separator>(separator) };
    }
};

struct unpack_fn
{
    template <class NextReducer>
    struct reducer_t
    {
        NextReducer m_next_reducer;

        template <class State, class Arg>
        step_t reduce(State& state, Arg&& arg) const
        {
            return std::apply(
                [&](auto&&... unpacked_args)
                { return m_next_reducer.reduce(state, std::forward<decltype(unpacked_args)>(unpacked_args)...); },
                std::forward<Arg>(arg));
        }
    };

    struct transducer_t
    {
        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            return reducer_t<std::decay_t<NextReducer>>{ std::forward<NextReducer>(next_reducer) };
        }
    };

    constexpr auto operator()() const { return transducer_t{}; }
};

struct project_fn
{
    template <class NextReducer, class... Funcs>
    struct reducer_t
    {
        NextReducer m_next_reducer;
        std::tuple<Funcs...> m_funcs;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return std::apply(
                [&](auto&&... funcs) -> step_t
                { return m_next_reducer.reduce(state, std::invoke(funcs, std::forward<Args>(args)...)...); },
                m_funcs);
        }
    };

    template <class... Funcs>
    struct transducer_t
    {
        std::tuple<Funcs...> m_funcs;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const
        {
            return reducer_t<std::decay_t<NextReducer>, Funcs...>{ std::forward<NextReducer>(next_reducer), m_funcs };
        }
    };

    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const -> transducer_t<std::decay_t<Funcs>...>
    {
        return { { std::forward<Funcs>(funcs)... } };
    }
};

}  // namespace detail

static constexpr inline auto transform = detail::transform_fn<false>{};
static constexpr inline auto transform_indexed = detail::transform_fn<true>{};
static constexpr inline auto filter = detail::filter_fn<false>{};
static constexpr inline auto filter_indexed = detail::filter_fn<true>{};
static constexpr inline auto take_while = detail::take_while_fn<false>{};
static constexpr inline auto drop_while = detail::drop_while_fn<false>{};
static constexpr inline auto drop_while_indexed = detail::drop_while_fn<true>{};
static constexpr inline auto take_while_indexed = detail::take_while_fn<true>{};
static constexpr inline auto take = detail::take_fn{};
static constexpr inline auto drop = detail::drop_fn{};

static constexpr inline auto join = detail::join_fn{}();
static constexpr inline auto intersperse = detail::intersperse_fn{};

static constexpr inline auto unpack = detail::unpack_fn{}();
static constexpr inline auto project = detail::project_fn{};

}  // namespace transducers

namespace reductors
{

namespace detail
{

struct copy_to_fn
{
    struct reducer_t
    {
        template <class State, class Arg>
        step_t reduce(State& state, Arg&& arg) const
        {
            *state = std::forward<Arg>(arg);
            ++state;
            return step_t::loop_continue;
        }
    };

    template <class OutputIt>
    constexpr auto operator()(OutputIt out) const
    {
        return reductor_t{ std::move(out), reducer_t{} };
    }
};

struct into_fn
{
    struct reducer_t
    {
        template <class Container, class Arg>
        step_t reduce(Container& state, Arg&& arg) const
        {
            state.push_back(std::forward<Arg>(arg));
            return step_t::loop_continue;
        }
    };

    template <class Container>
    constexpr auto operator()(Container&& container) const
    {
        return reductor_t{ std::forward<Container>(container), reducer_t{} };
    }
};

struct all_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr step_t reduce(bool& state, Args&&... args) const
        {
            state = state && std::invoke(m_pred, std::forward<Args>(args)...);
            return state ? step_t::loop_continue : step_t::loop_break;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { true, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct any_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr step_t reduce(bool& state, Args&&... args) const
        {
            state = state || std::invoke(m_pred, std::forward<Args>(args)...);
            return state ? step_t::loop_break : step_t::loop_continue;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { false, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct none_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr step_t reduce(bool& state, Args&&... args) const
        {
            state = state && !std::invoke(m_pred, std::forward<Args>(args)...);
            return state ? step_t::loop_continue : step_t::loop_break;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { true, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct fork_fn
{
    template <class... Reducers>
    struct reducer_t
    {
        std::tuple<Reducers...> m_reducers;
        mutable std::bitset<sizeof...(Reducers)> m_done{};

        template <std::size_t N, class State, class... Args>
        void call(State& state, Args&&... args) const
        {
            m_done[N] = (std::get<N>(m_reducers).reduce(std::get<N>(state), args...) == step_t::loop_break);
            if constexpr (N + 1 < sizeof...(Reducers))
            {
                call<N + 1>(state, args...);
            }
        }

        template <class State, class... Args>
        constexpr step_t reduce(State& state, Args&&... args) const
        {
            call<0>(state, args...);
            return !m_done.all() ? step_t::loop_continue : step_t::loop_break;
        }
    };

    template <class... Reducers>
    constexpr auto operator()(Reducers... reducers) const
        -> reductor_t<std::tuple<typename Reducers::state_type...>, reducer_t<typename Reducers::reducer_type...>>
    {
        return { { std::move(reducers.state)... },
                 {
                     { std::move(reducers.reducer)... },
                 } };
    }
};

struct sum_fn
{
    struct reducer_t
    {
        template <class State, class Arg>
        step_t reduce(State& state, Arg&& arg) const
        {
            state += std::forward<Arg>(arg);
            return step_t::loop_continue;
        }
    };

    template <class T>
    constexpr auto operator()(T init) const -> reductor_t<T, reducer_t>
    {
        return { init, reducer_t{} };
    }
};

struct count_fn
{
    struct reducer_t
    {
        template <class... Args>
        step_t reduce(std::size_t& state, Args&&...) const
        {
            ++state;
            return step_t::loop_continue;
        }
    };

    constexpr auto operator()() const -> reductor_t<std::size_t, reducer_t> { return { 0, reducer_t{} }; }
};

struct partition_fn
{
    template <class Pred, class OnTrueReducer, class OnFalseReducer>
    struct reducer_t
    {
        Pred m_pred;
        std::tuple<OnTrueReducer, OnFalseReducer> m_reducers;
        mutable std::bitset<2> m_done{};

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            if (std::invoke(m_pred, args...))
            {
                m_done[0]
                    = (std::get<0>(m_reducers).reduce(std::get<0>(state), std::forward<Args>(args)...)
                       == step_t::loop_break);
            }
            else
            {
                m_done[1]
                    = (std::get<1>(m_reducers).reduce(std::get<1>(state), std::forward<Args>(args)...)
                       == step_t::loop_break);
            }
            return !m_done.all() ? step_t::loop_continue : step_t::loop_break;
        }
    };

    template <class Pred, class S0, class R0, class S1, class R1>
    constexpr auto operator()(Pred&& pred, reductor_t<S0, R0> on_true_reducer, reductor_t<S1, R1> on_false_reducer) const
        -> reductor_t<std::tuple<S0, S1>, reducer_t<std::decay_t<Pred>, R0, R1>>
    {
        return { { std::move(on_true_reducer.state), std::move(on_false_reducer.state) },
                 { std::forward<Pred>(pred), { std::move(on_true_reducer.reducer), std::move(on_false_reducer.reducer) } } };
    }
};

struct accumulate_fn
{
    template <class Func>
    struct reducer_t
    {
        Func m_func;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            state = std::invoke(m_func, std::move(state), std::forward<Args>(args)...);
            return step_t::loop_continue;
        }
    };

    template <class State, class Func>
    constexpr auto operator()(State state, Func&& func) const
    {
        return reductor_t{ std::move(state), reducer_t<std::decay_t<Func>>{ std::forward<Func>(func) } };
    }
};

template <bool Indexed>
struct for_each_fn
{
    template <class Func>
    struct reducer_t
    {
        Func m_func;

        template <class... Args>
        step_t reduce(std::size_t& state, Args&&... args) const
        {
            if constexpr (Indexed)
            {
                std::invoke(m_func, state++, std::forward<Args>(args)...);
            }
            else
            {
                std::invoke(m_func, std::forward<Args>(args)...);
            }
            return step_t::loop_continue;
        }
    };
    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return reductor_t{ std::size_t{ 0 }, reducer_t<std::decay_t<Func>>{ std::forward<Func>(func) } };
    }
};

struct assign_fn
{
    template <class T>
    T& operator()(T& out, const T& in) const
    {
        if (&out != &in)
        {
            if constexpr (std::is_trivially_copy_assignable_v<T>)
            {
                out = in;
            }
            else
            {
                out.~T();
                ::new (static_cast<void*>(&out)) T(in);
            }
        }
        return out;
    }

    template <class T>
    T& operator()(T& out, T&& in) const
    {
        if (&out != &in)
        {
            if constexpr (std::is_trivially_copy_assignable_v<T>)
            {
                out = std::move(in);
            }
            else
            {
                out.~T();
                ::new (static_cast<void*>(&out)) T(std::move(in));
            }
        }
        return out;
    }
};

static constexpr inline auto assign = assign_fn{};

struct out_fn
{
    template <class State, class Reducer>
    struct iterator_t
    {
        using iterator_category = std::output_iterator_tag;
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
        using state_type = State;
        using reducer_type = Reducer;

        reductor_t<State, Reducer> m_reductor;

        constexpr iterator_t() = default;
        constexpr iterator_t(const iterator_t&) = default;
        constexpr iterator_t(iterator_t&&) = default;

        constexpr iterator_t& operator=(const iterator_t& other) { return assign(*this, other); }

        constexpr iterator_t& operator=(iterator_t&& other) { return assign(*this, std::move(other)); }

        constexpr iterator_t& operator*() { return *this; }

        constexpr iterator_t& operator++() { return *this; }

        constexpr iterator_t& operator++(int) { return *this; }

        template <class Arg, std::enable_if_t<!std::is_same_v<std::decay_t<Arg>, iterator_t>, int> = 0>
        constexpr iterator_t& operator=(Arg&& arg)
        {
            m_reductor(std::forward<Arg>(arg));
            return *this;
        }

        constexpr const state_type& get() const& { return m_reductor.state; }

        constexpr state_type& get() & { return m_reductor.state; }

        constexpr state_type&& get() && { return std::move(m_reductor.state); }
    };

    template <class State, class Reducer>
    constexpr auto operator()(reductor_t<State, Reducer> reductor) const -> iterator_t<State, Reducer>
    {
        return { std::move(reductor) };
    }
};

}  // namespace detail

static constexpr inline auto copy_to = detail::copy_to_fn{};
static constexpr inline auto into = detail::into_fn{};
static constexpr inline auto all_of = detail::all_of_fn{};
static constexpr inline auto any_of = detail::any_of_fn{};
static constexpr inline auto none_of = detail::none_of_fn{};
static constexpr inline auto fork = detail::fork_fn{};
static constexpr inline auto sum = detail::sum_fn{};
static constexpr inline auto count = detail::count_fn{}();
static constexpr inline auto partition = detail::partition_fn{};
static constexpr inline auto accumulate = detail::accumulate_fn{};
static constexpr inline auto out = detail::out_fn{};
static constexpr inline auto for_each = detail::for_each_fn<false>{};
static constexpr inline auto for_each_indexed = detail::for_each_fn<true>{};

}  // namespace reductors

using generators::chain;
using generators::from;
using generators::iota;
using generators::range;
using generators::repeat;

using generators::generate;
using generators::generator_t;

using transducers::drop;
using transducers::drop_while;
using transducers::drop_while_indexed;
using transducers::filter;
using transducers::filter_indexed;
using transducers::intersperse;
using transducers::join;
using transducers::project;
using transducers::take;
using transducers::take_while;
using transducers::take_while_indexed;
using transducers::transform;
using transducers::transform_indexed;
using transducers::unpack;

using reductors::accumulate;
using reductors::all_of;
using reductors::any_of;
using reductors::copy_to;
using reductors::count;
using reductors::for_each;
using reductors::for_each_indexed;
using reductors::fork;
using reductors::into;
using reductors::none_of;
using reductors::out;
using reductors::partition;
using reductors::sum;

}  // namespace zx
