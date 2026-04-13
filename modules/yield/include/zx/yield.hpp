#pragma once

#include <bitset>
#include <functional>
#include <tuple>

namespace zx
{

enum class step_t
{
    loop_continue,
    loop_break,
};

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
};

template <class State, class Reducer>
reductor_t(State&&, Reducer&&) -> reductor_t<std::decay_t<State>, std::decay_t<Reducer>>;

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

        template <class State, class Reducer>
        State yield_to(reductor_t<State, Reducer> reductor) const
        {
            for (T it = m_lower; it < m_upper; ++it)
            {
                if (reductor(it) == step_t::loop_break)
                {
                    break;
                }
            }
            return reductor.state;
        }
    };

    template <class T>
    constexpr auto operator()(T lower, T upper) const
    {
        return generator_t<T>{ lower, upper };
    }

    template <class T>
    constexpr auto operator()(T upper) const -> generator_t<T>
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

        template <class State, class Reducer>
        State yield_to(reductor_t<State, Reducer> reductor) const
        {
            for (T it = m_lower;; ++it)
            {
                if (reductor(it) == step_t::loop_break)
                {
                    break;
                }
            }
            return reductor.state;
        }
    };

    template <class T = std::ptrdiff_t>
    constexpr auto operator()(T lower = {}) const -> generator_t<T>
    {
        return { lower };
    }
};

struct from_fn
{
    template <class It>
    struct generator_t
    {
        It m_begin;
        It m_end;

        template <class State, class Reducer>
        State yield_to(reductor_t<State, Reducer> reductor) const
        {
            for (It it = m_begin; it != m_end; ++it)
            {
                if (reductor(*it) == step_t::loop_break)
                {
                    break;
                }
            }
            return reductor.state;
        }
    };

    template <class Range>
    constexpr auto operator()(Range&& range) const -> generator_t<decltype(std::begin(range))>
    {
        return { std::begin(range), std::end(range) };
    }
};

}  // namespace detail

static constexpr inline auto range = detail::range_fn{};
static constexpr inline auto iota = detail::iota_fn{};
static constexpr inline auto from = detail::from_fn{};

}  // namespace generators

namespace transducers
{

namespace detail
{
struct transform_fn
{
    template <class Func, class NextReducer>
    struct reducer_t
    {
        Func m_func;
        NextReducer m_next_reducer;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            return m_next_reducer.reduce(state, std::invoke(m_func, std::forward<Args>(args)...));
        }
    };

    template <class Func>
    struct transducer_t
    {
        Func m_func;

        template <class Reducer>
        constexpr auto transduce(Reducer&& reducer) const&
        {
            return reducer_t<Func, std::decay_t<Reducer>>{ m_func, std::forward<Reducer>(reducer) };
        }

        template <class Reducer>
        constexpr auto transduce(Reducer&& reducer) &&
        {
            return reducer_t<Func, std::decay_t<Reducer>>{ std::move(m_func), std::forward<Reducer>(reducer) };
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return transducer_t<std::decay_t<Func>>{ std::forward<Func>(func) };
    }
};

struct filter_fn
{
    template <class Pred, class NextReducer>
    struct reducer_t
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

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ std::move(m_pred), std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return transducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct take_while_fn
{
    template <class Pred, class NextReducer>
    struct reducer_t
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, std::forward<Args>(args)...);
            if (!m_done)
            {
                return m_next_reducer.reduce(state, std::forward<Args>(args)...);
            }
            return step_t::loop_break;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ std::move(m_pred), std::forward<NextReducer>(next_reducer) };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return transducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct drop_while_fn
{
    template <class Pred, class NextReducer>
    struct reducer_t
    {
        Pred m_pred;
        NextReducer m_next_reducer;
        mutable bool m_done = false;

        template <class State, class... Args>
        step_t reduce(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, std::forward<Args>(args)...);
            if (m_done)
            {
                return m_next_reducer.reduce(state, std::forward<Args>(args)...);
            }
            return step_t::loop_continue;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) const&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ m_pred, std::forward<NextReducer>(next_reducer) };
        }

        template <class NextReducer>
        constexpr auto transduce(NextReducer&& next_reducer) &&
        {
            return reducer_t<Pred, std::decay_t<NextReducer>>{ std::move(m_pred), std::forward<NextReducer>(next_reducer) };
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

}  // namespace detail

static constexpr inline auto transform = detail::transform_fn{};
static constexpr inline auto filter = detail::filter_fn{};
static constexpr inline auto take_while = detail::take_while_fn{};
static constexpr inline auto drop_while = detail::drop_while_fn{};
static constexpr inline auto take = detail::take_fn{};
static constexpr inline auto drop = detail::drop_fn{};

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
        template <class State, class... Args>
        step_t reduce(State& state, Args&&...) const
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

}  // namespace detail

static constexpr inline auto copy_to = detail::copy_to_fn{};
static constexpr inline auto into = detail::into_fn{};
static constexpr inline auto all_of = detail::all_of_fn{};
static constexpr inline auto any_of = detail::any_of_fn{};
static constexpr inline auto none_of = detail::none_of_fn{};
static constexpr inline auto fork = detail::fork_fn{};
static constexpr inline auto sum = detail::sum_fn{};
static constexpr inline auto count = detail::count_fn{};
static constexpr inline auto partition = detail::partition_fn{};

}  // namespace reductors

using generators::from;
using generators::iota;
using generators::range;

using transducers::drop;
using transducers::drop_while;
using transducers::filter;
using transducers::take;
using transducers::take_while;
using transducers::transform;

using reductors::all_of;
using reductors::any_of;
using reductors::copy_to;
using reductors::count;
using reductors::fork;
using reductors::into;
using reductors::none_of;
using reductors::partition;
using reductors::sum;

}  // namespace zx
