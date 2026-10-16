#pragma once

#include <limits>
#include <memory>
#include <utility>
#include <zx/iterator_interface.hpp>
#include <zx/maybe.hpp>
#include <zx/pipe.hpp>

namespace zx
{

template <class T>
using iteration_result_t = maybe_t<T>;

template <class T>
using next_function_t = std::function<iteration_result_t<T>()>;

template <class T, class NextFn = next_function_t<T>>
struct sequence_t;

namespace detail
{

template <class Func>
struct default_constructible_func
{
    static constexpr bool is_default_constructible = std::is_default_constructible_v<Func>;

    using impl_type = std::conditional_t<is_default_constructible, Func, std::optional<Func>>;

    constexpr default_constructible_func() = default;

    constexpr default_constructible_func(Func func) : m_func{ std::move(func) } { }

    constexpr default_constructible_func(const default_constructible_func&) = default;

    constexpr default_constructible_func(default_constructible_func&&) = default;

    constexpr default_constructible_func& operator=(default_constructible_func other)
    {
        if constexpr (std::is_move_assignable_v<impl_type>)
        {
            m_func = std::move(other.m_func);
        }
        else
        {
            m_func.~impl_type();
            new (&m_func) impl_type{ std::move(other.m_func) };
        }
        return *this;
    }

    template <class... Args>
    auto operator()(Args&&... args) const -> std::invoke_result_t<Func, Args...>
    {
        if constexpr (is_default_constructible)
        {
            return std::invoke(m_func, std::forward<Args>(args)...);
        }
        else
        {
            return std::invoke(*m_func, std::forward<Args>(args)...);
        }
    }

    mutable impl_type m_func;
};

template <class T, class NextFn>
struct sequence_iterator
{
    using next_function_type = NextFn;
    using reference = T;
    using difference_type = std::ptrdiff_t;
    using value_type = std::decay_t<reference>;
    using pointer = std::conditional_t<  //
        std::is_reference_v<reference>,
        std::add_pointer_t<reference>,
        detail::pointer_proxy<reference>>;
    using iterator_category = std::input_iterator_tag;

    default_constructible_func<next_function_type> m_next_fn;
    iteration_result_t<reference> m_current;
    difference_type m_index;

    sequence_iterator() : m_next_fn{}, m_current{}, m_index{ std::numeric_limits<difference_type>::max() } { }

    sequence_iterator(const next_function_type& next_fn) : m_next_fn{ next_fn }, m_current{ m_next_fn() }, m_index{ 0 } { }

    sequence_iterator(const sequence_iterator&) = default;
    sequence_iterator(sequence_iterator&&) noexcept = default;

    sequence_iterator& operator=(sequence_iterator other)
    {
        std::swap(m_next_fn, other.m_next_fn);
        return *this;
    }

    reference operator*() const { return *m_current; }

    pointer operator->() const
    {
        if constexpr (std::is_reference_v<reference>)
        {
            return std::addressof(**this);
        }
        else
        {
            return pointer{ **this };
        }
    }

    sequence_iterator& operator++()
    {
        m_current = m_next_fn();
        ++m_index;
        return *this;
    }

    sequence_iterator operator++(int)
    {
        sequence_iterator temp{ *this };
        ++(*this);
        return temp;
    }

    friend bool operator==(const sequence_iterator& lhs, const sequence_iterator& rhs)
    {
        return (!lhs.m_current && !rhs.m_current) || (lhs.m_current && rhs.m_current && lhs.m_index == rhs.m_index);
    }

    friend bool operator!=(const sequence_iterator& lhs, const sequence_iterator& rhs) { return !(lhs == rhs); }
};

template <class T>
struct empty_sequence
{
    auto operator()() const -> iteration_result_t<T> { return {}; }
};

template <class To, class From, class NextFn>
struct cast_sequence
{
    NextFn m_next;

    auto operator()() const -> iteration_result_t<To>
    {
        iteration_result_t<From> value = m_next();
        if (value)
        {
            return static_cast<To>(*value);
        }
        return {};
    }
};

template <class Iter, class Out>
struct view_sequence
{
    mutable Iter m_iter;
    Iter m_end;

    auto operator()() const -> iteration_result_t<Out>
    {
        if (m_iter == m_end)
        {
            return {};
        }
        return *m_iter++;
    }
};

template <class Range, class Iter, class Out>
struct owning_sequence
{
    std::shared_ptr<Range> m_range;
    mutable Iter m_iter;

    explicit owning_sequence(std::shared_ptr<Range> range) : m_range(range), m_iter(std::begin(*m_range)) { }

    auto operator()() const -> iteration_result_t<Out>
    {
        if (m_iter == std::end(*m_range))
        {
            return {};
        }
        return *m_iter++;
    }
};

template <class T>
struct is_sequence : std::false_type
{
};

template <class T, class NextFn>
struct is_sequence<sequence_t<T, NextFn>> : std::true_type
{
};

}  // namespace detail

template <class T, class NextFn>
struct sequence_t
{
    static_assert(!std::is_rvalue_reference_v<T>, "sequence_t element type must not be an rvalue reference");

    using iterator = detail::sequence_iterator<T, NextFn>;
    using next_function_type = typename iterator::next_function_type;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer = typename iterator::pointer;
    using difference_type = typename iterator::difference_type;

    next_function_type m_next_fn;

    explicit sequence_t(next_function_type next_fn) : m_next_fn(std::move(next_fn)) { }

    template <class N, enable_if_t<std::is_constructible_v<next_function_type, N>> = 0>
    sequence_t(const sequence_t<reference, N>& other) : sequence_t{ other.get_next_function() }
    {
    }

    template <class N, enable_if_t<std::is_constructible_v<next_function_type, N>> = 0>
    sequence_t(sequence_t<reference, N>&& other) : sequence_t{ std::move(other).get_next_function() }
    {
    }

    template <class U, class N, enable_if_t<std::is_constructible_v<reference, U>> = 0>
    sequence_t(const sequence_t<U, N>& other)
        : sequence_t{ detail::cast_sequence<reference, U, N>{ other.get_next_function() } }
    {
    }

    template <class U, class N, enable_if_t<std::is_constructible_v<reference, U>> = 0>
    sequence_t(sequence_t<U, N>&& other)
        : sequence_t{ detail::cast_sequence<reference, U, N>{ std::move(other).get_next_function() } }
    {
    }

    template <class Iter, enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    explicit sequence_t(Iter b, Iter e) : sequence_t{ detail::view_sequence<Iter, reference>{ b, e } }
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    sequence_t(Range&& range) : sequence_t{ std::begin(range), std::end(range) }
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    sequence_t(Range range, int)
        : sequence_t{ detail::owning_sequence<Range, Iter, reference>{ std::make_shared<Range>(std::move(range)) } }
    {
    }

    template <class... Args>
    sequence_t(std::in_place_t, Args&&... args) : sequence_t{ next_function_type{ std::forward<Args>(args)... } }
    {
    }

    sequence_t() : sequence_t(detail::empty_sequence<T>{}) { }

    template <
        class Container,
        enable_if_t<std::is_constructible_v<Container, iterator, iterator>, !detail::is_sequence<Container>::value> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto begin() const -> iterator { return iterator{ m_next_fn }; }

    auto end() const -> iterator { return iterator{}; }

    auto get_next_function() const& -> const next_function_type& { return m_next_fn; }

    auto get_next_function() && -> next_function_type&& { return std::move(m_next_fn); }

    auto maybe_front() const& -> maybe_t<reference> { return get_next_function()(); }

    auto front() const& -> reference { return maybe_front().value(); }

    auto maybe_at(difference_type n) const -> maybe_t<reference> { return this->drop(n).maybe_front(); }

    auto at(difference_type n) const -> reference { return maybe_at(n).value(); }

    bool empty() const { return begin() == end(); }
};

namespace detail
{

struct iota_fn
{
    template <class In>
    struct next_function
    {
        mutable In m_current;
        auto operator()() const -> iteration_result_t<In> { return m_current++; }
    };

    template <class T, class Seq = sequence_t<T, next_function<T>>>
    auto operator()(T init) const -> Seq
    {
        return Seq{ std::in_place, init };
    }
};

struct range_fn
{
    template <class In>
    struct next_function
    {
        mutable In m_current;
        In m_upper;

        auto operator()() const -> iteration_result_t<In>
        {
            if (m_current >= m_upper)
            {
                return {};
            }
            return m_current++;
        }
    };

    template <class T, class Seq = sequence_t<T, next_function<T>>>
    auto operator()(T lower, T upper) const -> Seq
    {
        return Seq{ std::in_place, lower, upper };
    }

    template <class T, class Seq = sequence_t<T, next_function<T>>>
    auto operator()(T upper) const -> Seq
    {
        return (*this)(T{}, upper);
    }
};

struct unfold_fn
{
    template <class Func, class S, class Out>
    struct next_function
    {
        Func m_func;
        mutable S m_state;

        auto operator()() const -> iteration_result_t<Out>
        {
            auto res = m_func(m_state);
            if (!res)
            {
                return {};
            }
            auto&& [value, new_state] = *std::move(res);
            m_state = std::move(new_state);
            return value;
        }
    };

    template <
        class S,
        class Func,
        class OptRes = std::invoke_result_t<Func, const S&>,
        class Res = maybe_underlying_type_t<OptRes>,
        class Out = std::tuple_element_t<0, Res>,
        class Seq = sequence_t<Out, next_function<std::decay_t<Func>, S, Out>>>
    auto operator()(S state, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(state) };
    }
};

struct view_fn
{
    template <
        class Range,
        class Out = range_reference_t<Range>,
        class Seq = sequence_t<Out, view_sequence<iterator_t<Range>, Out>>>
    auto operator()(Range&& range) const -> Seq
    {
        return Seq{ std::in_place, std::begin(range), std::end(range) };
    }

    template <class Iter, class Out = iter_reference_t<Iter>, class Seq = sequence_t<Out, view_sequence<Iter, Out>>>
    auto operator()(Iter b, Iter e) const -> Seq
    {
        return Seq{ std::in_place, b, e };
    }

    template <class T, class NextFn>
    auto operator()(const sequence_t<T, NextFn>& seq) const -> sequence_t<T, NextFn>
    {
        return seq;
    }

    template <class T, class NextFn>
    auto operator()(sequence_t<T, NextFn>&& seq) const -> sequence_t<T, NextFn>
    {
        return seq;
    }
};

struct owning_fn
{
    template <
        class Range,
        class Out = range_reference_t<Range>,
        class Seq = sequence_t<Out, owning_sequence<Range, iterator_t<Range>, Out>>>
    auto operator()(Range range) const -> Seq
    {
        return Seq{ std::in_place, std::make_shared<Range>(std::move(range)) };
    }

    template <class T, class NextFn>
    auto operator()(const sequence_t<T, NextFn>& s) const -> sequence_t<T, NextFn>
    {
        return s;
    }

    template <class T, class NextFn>
    auto operator()(sequence_t<T, NextFn>&& s) const -> sequence_t<T, NextFn>
    {
        return s;
    }
};

struct single_fn
{
    template <class T>
    struct next_function
    {
        T m_value;
        mutable bool m_init = true;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_init)
            {
                m_init = false;
                return m_value;
            }
            return {};
        }
    };

    template <class T, class Seq = sequence_t<T, next_function<T>>>
    auto operator()(T value) const -> Seq
    {
        return Seq{ std::in_place, std::move(value) };
    }
};

struct repeat_fn
{
    template <class T>
    struct next_function
    {
        T m_value;

        auto operator()() const -> iteration_result_t<T> { return m_value; }
    };

    template <class T, class Seq = sequence_t<T, next_function<T>>>
    auto operator()(T value) const -> Seq
    {
        return Seq{ std::in_place, std::move(value) };
    }
};

struct concat_fn
{
    template <class T, class N0, class N1>
    struct next_function
    {
        N0 m_first;
        N1 m_second;
        mutable bool m_first_finished = false;

        auto operator()() const -> iteration_result_t<T>
        {
            if (!m_first_finished)
            {
                iteration_result_t<T> n = m_first();
                if (n)
                {
                    return n;
                }
                else
                {
                    m_first_finished = true;
                }
            }
            return m_second();
        }
    };

    template <
        class T0,
        class N0,
        class T1,
        class N1,
        class T2,
        class N2,
        class T3,
        class N3,
        class Out = std::common_type_t<T0, T1, T2, T3>>
    auto operator()(
        const sequence_t<T0, N0>& s0,
        const sequence_t<T1, N1>& s1,
        const sequence_t<T2, N2>& s2,
        const sequence_t<T3, N3>& s3) const
    {
        return (*this)((*this)(s0, s1, s2), s3);
    }

    template <class T0, class N0, class T1, class N1, class T2, class N2, class Out = std::common_type_t<T0, T1, T2>>
    auto operator()(const sequence_t<T0, N0>& s0, const sequence_t<T1, N1>& s1, const sequence_t<T2, N2>& s2) const
    {
        return (*this)((*this)(s0, s1), s2);
    }

    template <class T0, class N0, class T1, class N1, class Out = std::common_type_t<T0, T1>>
    auto operator()(const sequence_t<T0, N0>& s0, const sequence_t<T1, N1>& s1) const
    {
        return (*this)(sequence_t<Out>{ s0 }, sequence_t<Out>{ s1 });
    }

    template <class T, class N0, class N1, class Seq = sequence_t<T, next_function<T, N0, N1>>>
    auto operator()(const sequence_t<T, N0>& lhs, const sequence_t<T, N1>& rhs) const -> Seq
    {
        return Seq{ std::in_place, lhs.get_next_function(), rhs.get_next_function() };
    }
};

struct zip_fn
{
    template <class T0, class N0, class T1, class N1, class T2 = void, class N2 = void, class T3 = void, class N3 = void>
    struct next_function;

    template <class T0, class N0, class T1, class N1, class T2, class N2, class T3, class N3>
    struct next_function
    {
        N0 m_next0;
        N1 m_next1;
        N2 m_next2;
        N3 m_next3;

        auto operator()() const -> iteration_result_t<std::tuple<T0, T1, T2, T3>>
        {
            iteration_result_t<T0> n0 = m_next0();
            iteration_result_t<T1> n1 = m_next1();
            iteration_result_t<T2> n2 = m_next2();
            iteration_result_t<T3> n3 = m_next3();
            if (n0 && n1 && n2 && n3)
            {
                return std::tuple<T0, T1, T2, T3>{ *n0, *n1, *n2, *n3 };
            }
            return {};
        }
    };

    template <class T0, class N0, class T1, class N1, class T2, class N2>
    struct next_function<T0, N0, T1, N1, T2, N2, void, void>
    {
        N0 m_next0;
        N1 m_next1;
        N2 m_next2;

        auto operator()() const -> iteration_result_t<std::tuple<T0, T1, T2>>
        {
            iteration_result_t<T0> n0 = m_next0();
            iteration_result_t<T1> n1 = m_next1();
            iteration_result_t<T2> n2 = m_next2();
            if (n0 && n1 && n2)
            {
                return std::tuple<T0, T1, T2>{ *n0, *n1, *n2 };
            }
            return {};
        }
    };

    template <class T0, class N0, class T1, class N1>
    struct next_function<T0, N0, T1, N1, void, void, void, void>
    {
        N0 m_next0;
        N1 m_next1;

        auto operator()() const -> iteration_result_t<std::tuple<T0, T1>>
        {
            iteration_result_t<T0> n0 = m_next0();
            iteration_result_t<T1> n1 = m_next1();
            if (n0 && n1)
            {
                return std::tuple<T0, T1>{ *n0, *n1 };
            }
            return {};
        }
    };

    template <
        class T0,
        class N0,
        class T1,
        class N1,
        class T2,
        class N2,
        class T3,
        class N3,
        class Out = std::tuple<T0, T1, T2, T3>,
        class Seq = sequence_t<Out, next_function<T0, N0, T1, N1, T2, N2, T3, N3>>>
    auto operator()(
        const sequence_t<T0, N0>& s0,
        const sequence_t<T1, N1>& s1,
        const sequence_t<T2, N2>& s2,
        const sequence_t<T3, N3>& s3) const -> Seq
    {
        return Seq{
            std::in_place, s0.get_next_function(), s1.get_next_function(), s2.get_next_function(), s3.get_next_function()
        };
    }

    template <
        class T0,
        class N0,
        class T1,
        class N1,
        class T2,
        class N2,
        class Out = std::tuple<T0, T1, T2>,
        class Seq = sequence_t<Out, next_function<T0, N0, T1, N1, T2, N2>>>
    auto operator()(const sequence_t<T0, N0>& s0, const sequence_t<T1, N1>& s1, const sequence_t<T2, N2>& s2) const -> Seq
    {
        return Seq{ std::in_place, s0.get_next_function(), s1.get_next_function(), s2.get_next_function() };
    }

    template <
        class T0,
        class N0,
        class T1,
        class N1,
        class Out = std::tuple<T0, T1>,
        class Seq = sequence_t<Out, next_function<T0, N0, T1, N1>>>
    auto operator()(const sequence_t<T0, N0>& s0, const sequence_t<T1, N1>& s1) const -> Seq
    {
        return Seq{ std::in_place, s0.get_next_function(), s1.get_next_function() };
    }
};

struct vec_fn
{
    template <class T, class... Tail>
    auto operator()(T head, Tail&&... tail) const
    {
        return owning_fn{}(std::vector<T>{ std::move(head), std::forward<Tail>(tail)... });
    }
};

template <class T>
struct empty_fn
{
    auto operator()() const -> sequence_t<T, empty_sequence<T>> { return sequence_t<T, empty_sequence<T>>{}; }
};

struct transform_fn
{
    template <class T, class NextFn, class Func, class Out>
    struct next_function
    {
        Func m_func;
        NextFn m_next;

        auto operator()() const -> iteration_result_t<Out>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                return std::invoke(m_func, *std::move(next));
            }
            return {};
        }
    };

    template <
        class T,
        class NextFn,
        class Func,
        class Res = remove_rvalue_reference_t<std::invoke_result_t<Func, T>>,
        class Seq = sequence_t<Res, next_function<T, NextFn, std::decay_t<Func>, Res>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct transform_indexed_fn
{
    template <class T, class NextFn, class Func, class Out>
    struct next_function
    {
        Func m_func;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<Out>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                return std::invoke(m_func, m_index++, *std::move(next));
            }
            return {};
        }
    };

    template <
        class T,
        class NextFn,
        class Func,
        class Res = remove_rvalue_reference_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>,
        class Seq = sequence_t<Res, next_function<T, NextFn, std::decay_t<Func>, Res>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct transform_maybe_fn
{
    template <class T, class NextFn, class Func, class Out>
    struct next_function
    {
        Func m_func;
        NextFn m_next;

        auto operator()() const -> iteration_result_t<Out>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                iteration_result_t<Out> r = std::invoke(m_func, *std::move(res));
                if (r)
                {
                    return r;
                }
            }
            return {};
        }
    };

    template <
        class T,
        class NextFn,
        class Func,
        class Res = remove_rvalue_reference_t<maybe_underlying_type_t<std::invoke_result_t<Func, T>>>,
        class Seq = sequence_t<Res, next_function<T, NextFn, std::decay_t<Func>, Res>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct transform_maybe_indexed_fn
{
    template <class T, class NextFn, class Func, class Out>
    struct next_function
    {
        Func m_func;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<Out>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                iteration_result_t<Out> r = std::invoke(m_func, m_index++, *std::move(res));
                if (r)
                {
                    return r;
                }
            }
            return {};
        }
    };

    template <
        class T,
        class NextFn,
        class Func,
        class Res = remove_rvalue_reference_t<maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>,
        class Seq = sequence_t<Res, next_function<T, NextFn, std::decay_t<Func>, Res>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct filter_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (std::invoke(m_pred, *res))
                {
                    return res;
                }
            }
            return {};
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct filter_indexed_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (std::invoke(m_pred, m_index++, *res))
                {
                    return res;
                }
            }
            return {};
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct drop_while_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;
        mutable bool m_init = true;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_init)
            {
                while (true)
                {
                    iteration_result_t<T> res = m_next();
                    if (!res)
                    {
                        return {};
                    }
                    if (!std::invoke(m_pred, *res))
                    {
                        m_init = false;
                        return res;
                    }
                }
            }
            return m_next();
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct drop_while_indexed_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;
        mutable bool m_init = true;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_init)
            {
                while (true)
                {
                    iteration_result_t<T> res = m_next();
                    if (!res)
                    {
                        return {};
                    }
                    if (!std::invoke(m_pred, m_index++, *res))
                    {
                        m_init = false;
                        return res;
                    }
                }
            }
            return m_next();
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const& -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct take_while_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;
        mutable bool m_init = true;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> res = m_next();
            if (!(res && std::invoke(m_pred, *res)))
            {
                return {};
            }
            return res;
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct take_while_indexed_fn
{
    template <class T, class NextFn, class Pred>
    struct next_function
    {
        Pred m_pred;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> res = m_next();
            if (!(res && std::invoke(m_pred, m_index++, *res)))
            {
                return {};
            }
            return res;
        }
    };

    template <class T, class NextFn, class Pred, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Pred>>>>
    auto operator()(sequence_t<T, NextFn> value, Pred&& pred) const& -> Seq
    {
        return Seq{ std::in_place, std::forward<Pred>(pred), std::move(value).get_next_function() };
    }

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Pred>(pred)));
    }
};

struct drop_fn
{
    template <class T, class NextFn>
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        NextFn m_next;
        mutable bool m_init = false;

        auto operator()() const -> iteration_result_t<T>
        {
            if (!m_init)
            {
                while (m_count > 0)
                {
                    --m_count;
                    m_next();
                }
                m_init = true;
            }
            return m_next();
        }
    };

    template <class T, class NextFn, class Seq = sequence_t<T, next_function<T, NextFn>>>
    auto operator()(sequence_t<T, NextFn> value, std::ptrdiff_t n) const -> Seq
    {
        return Seq{ std::in_place, n, std::move(value).get_next_function() };
    }

    auto operator()(std::ptrdiff_t n) const { return fn(std::bind(*this, std::placeholders::_1, n)); }
};

struct take_fn
{
    template <class T, class NextFn>
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        NextFn m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            if (m_count == 0)
            {
                return {};
            }
            --m_count;
            return m_next();
        }
    };

    template <class T, class NextFn, class Seq = sequence_t<T, next_function<T, NextFn>>>
    auto operator()(sequence_t<T, NextFn> value, std::ptrdiff_t n) const -> Seq
    {
        return Seq{ std::in_place, n, std::move(value).get_next_function() };
    }

    auto operator()(std::ptrdiff_t n) const { return fn(std::bind(*this, std::placeholders::_1, n)); }
};

struct step_fn
{
    template <class T, class NextFn>
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                iteration_result_t<T> res = m_next();
                if (!res)
                {
                    break;
                }

                if (m_index++ % m_count == 0)
                {
                    return res;
                }
            }
            return {};
        }
    };

    template <class T, class NextFn, class Seq = sequence_t<T, next_function<T, NextFn>>>
    auto operator()(sequence_t<T, NextFn> value, std::ptrdiff_t n) const -> Seq
    {
        return Seq{ std::in_place, n, std::move(value).get_next_function() };
    }

    auto operator()(std::ptrdiff_t n) const { return fn(std::bind(*this, std::placeholders::_1, n)); }
};

struct inspect_fn
{
    template <class T, class NextFn, class Func>
    struct next_function
    {
        Func m_func;
        NextFn m_next;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                std::invoke(m_func, *next);
            }
            return next;
        }
    };

    template <class T, class NextFn, class Func, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Func>>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct inspect_indexed_fn
{
    template <class T, class NextFn, class Func>
    struct next_function
    {
        Func m_func;
        NextFn m_next;
        mutable std::ptrdiff_t m_index = 0;

        auto operator()() const -> iteration_result_t<T>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                std::invoke(m_func, m_index++, *next);
            }
            return next;
        }
    };

    template <class T, class NextFn, class Func, class Seq = sequence_t<T, next_function<T, NextFn, std::decay_t<Func>>>>
    auto operator()(sequence_t<T, NextFn> value, Func&& func) const -> Seq
    {
        return Seq{ std::in_place, std::forward<Func>(func), std::move(value).get_next_function() };
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct intersperse_fn
{
    enum class state_t
    {
        start,
        yield_item,
        yield_separator
    };

    template <class T, class NextFn, class E, class Out>
    struct next_function
    {
        NextFn m_next;
        E m_element;
        mutable state_t m_state = state_t::start;
        mutable iteration_result_t<Out> m_next_item = {};

        auto operator()() const -> iteration_result_t<Out>
        {
            switch (m_state)
            {
                case state_t::start:
                    m_next_item = m_next();
                    if (m_next_item)
                    {
                        m_state = state_t::yield_separator;
                        return *m_next_item;
                    }
                    else
                    {
                        return {};
                    }
                case state_t::yield_item: m_state = state_t::yield_separator; return m_next_item;
                case state_t::yield_separator:
                    m_state = state_t::yield_item;
                    m_next_item = m_next();
                    if (m_next_item)
                    {
                        return m_element;
                    }
                    else
                    {
                        return {};
                    }
            }
            return {};
        }
    };

    template <
        class T,
        class NextFn,
        class E,
        class Out = std::common_type_t<T, E>,
        class Seq = sequence_t<Out, next_function<T, NextFn, E, Out>>>
    auto operator()(sequence_t<T, NextFn> value, E element) const -> Seq
    {
        return Seq{ std::in_place, std::move(value).get_next_function(), std::move(element) };
    }

    template <class E>
    auto operator()(E element) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::move(element)));
    }
};

struct join_fn
{
    template <class T, class InnerNextFn, class OuterNextFn>
    struct next_function
    {
        OuterNextFn m_next;
        mutable std::optional<InnerNextFn> m_sub = {};

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                if (!m_sub)
                {
                    iteration_result_t<sequence_t<T, InnerNextFn>> next = m_next();
                    if (!next)
                    {
                        return {};
                    }
                    m_sub = next->get_next_function();
                    continue;
                }
                iteration_result_t<T> next_sub = (*m_sub)();
                if (next_sub)
                {
                    return next_sub;
                }
                else
                {
                    m_sub = {};
                    continue;
                }
            }
            return {};
        }
    };

    template <
        class T,
        class InnerNextFn,
        class OuterNextFn,
        class Seq = sequence_t<T, next_function<T, InnerNextFn, OuterNextFn>>>
    auto operator()(sequence_t<sequence_t<T, InnerNextFn>, OuterNextFn> value) const -> Seq
    {
        return Seq{ std::in_place, std::move(value).get_next_function() };
    }

    auto operator()() const { return fn(std::bind(*this, std::placeholders::_1)); }
};

struct for_each_fn
{
    template <class T, class NextFn, class Func>
    Func operator()(const sequence_t<T, NextFn>& value, Func func) const
    {
        const auto next_function = value.get_next_function();
        while (true)
        {
            const iteration_result_t<T> next = next_function();
            if (!next)
            {
                break;
            }
            std::invoke(func, *next);
        }
        return func;
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct for_each_indexed_fn
{
    template <class T, class NextFn, class Func>
    Func operator()(const sequence_t<T, NextFn>& value, Func func) const
    {
        const auto next_function = value.get_next_function();
        std::ptrdiff_t index = 0;
        while (true)
        {
            const iteration_result_t<T> next = next_function();
            if (!next)
            {
                break;
            }
            std::invoke(func, index++, *next);
        }
        return func;
    }

    template <class Func>
    auto operator()(Func&& func) const
    {
        return fn(std::bind(*this, std::placeholders::_1, std::forward<Func>(func)));
    }
};

struct init_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(std::ptrdiff_t n, Func&& func) const
    {
        static const auto range = range_fn{};
        static const auto transform = transform_fn{};
        return range(n) |= transform(std::forward<Func>(func));
    }
};

struct init_infinite_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(Func&& func) const
    {
        static const auto iota = iota_fn{};
        static const auto transform = transform_fn{};
        return iota(std::ptrdiff_t{ 0 }) |= transform(std::forward<Func>(func));
    }
};

struct linspace_fn
{
    template <class T>
    auto operator()(T start, T end, std::size_t count, bool endpoint = true) const
    {
        static const auto init = init_fn{};
        const auto step = endpoint ? (end - start) / static_cast<T>(count - 1) : (end - start) / static_cast<T>(count);
        return init(
            static_cast<std::ptrdiff_t>(count),
            [start, step](std::ptrdiff_t i) { return start + static_cast<T>(i) * step; });
    }
};

}  // namespace detail

namespace seq
{

template <class T>
static const inline auto empty = detail::empty_fn<T>{};

inline constexpr auto iota = detail::iota_fn{};
inline constexpr auto range = detail::range_fn{};
inline constexpr auto unfold = detail::unfold_fn{};
inline constexpr auto view = detail::view_fn{};
inline constexpr auto owning = detail::owning_fn{};
inline constexpr auto repeat = detail::repeat_fn{};
inline constexpr auto single = detail::single_fn{};
inline constexpr auto concat = detail::concat_fn{};
inline constexpr auto vec = detail::vec_fn{};
inline constexpr auto zip = detail::zip_fn{};
inline constexpr auto init = detail::init_fn{};
inline constexpr auto linspace = detail::linspace_fn{};
inline constexpr auto init_infinite = detail::init_infinite_fn{};

inline constexpr auto transform = detail::transform_fn{};
inline constexpr auto transform_indexed = detail::transform_indexed_fn{};
inline constexpr auto transform_maybe = detail::transform_maybe_fn{};
inline constexpr auto transform_maybe_indexed = detail::transform_maybe_indexed_fn{};

inline constexpr auto filter = detail::filter_fn{};
inline constexpr auto filter_indexed = detail::filter_indexed_fn{};

inline constexpr auto drop_while = detail::drop_while_fn{};
inline constexpr auto drop_while_indexed = detail::drop_while_indexed_fn{};

inline constexpr auto take_while = detail::take_while_fn{};
inline constexpr auto take_while_indexed = detail::take_while_indexed_fn{};

inline constexpr auto take = detail::take_fn{};
inline constexpr auto drop = detail::drop_fn{};
inline constexpr auto step = detail::step_fn{};

inline constexpr auto inspect = detail::inspect_fn{};
inline constexpr auto inspect_indexed = detail::inspect_indexed_fn{};

inline constexpr auto intersperse = detail::intersperse_fn{};

inline constexpr auto join = detail::join_fn{};

inline constexpr auto for_each = detail::for_each_fn{};
inline constexpr auto for_each_indexed = detail::for_each_indexed_fn{};

}  // namespace seq

}  // namespace zx
