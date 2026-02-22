#pragma once

#include <zx/iterator_interface.hpp>
#include <zx/maybe.hpp>

namespace zx
{

template <class T>
using iteration_result_t = maybe_t<T>;

template <class T>
using next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
struct sequence_t;

namespace detail
{

template <class T>
struct inspect_mixin
{
    template <class Func>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

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

    template <class Func>
    auto inspect(Func&& func) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect(Func&& func) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct inspect_indexed_mixin
{
    template <class Func>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
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

    template <class Func>
    auto inspect_indexed(Func&& func) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect_indexed(Func&& func) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

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

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto transform(Func&& func) const& -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto transform(Func&& func) && -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_indexed_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
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

    template <class Func, class Res = std::invoke_result_t<Func, std::ptrdiff_t, T>>
    auto transform_indexed(Func&& func) const& -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, std::ptrdiff_t, T>>
    auto transform_indexed(Func&& func) && -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_maybe_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

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

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, T>>>
    auto transform_maybe(Func&& func) const& -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, T>>>
    auto transform_maybe(Func&& func) && -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct transform_maybe_indexed_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;
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

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>
    auto transform_maybe_indexed(Func&& func) const& -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>
    auto transform_maybe_indexed(Func&& func) && -> sequence_t<Res>
    {
        return sequence_t<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct filter_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;

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

    template <class Pred>
    auto filter(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct filter_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
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

    template <class Pred>
    auto filter_indexed(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter_indexed(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_while_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
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

    template <class Pred>
    auto drop_while(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_while_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
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

    template <class Pred>
    auto drop_while_indexed(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while_indexed(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_while_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;

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

    template <class Pred>
    auto take_while(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_while_indexed_mixin
{
    template <class Pred>
    struct next_function
    {
        Pred m_pred;
        next_function_t<T> m_next;
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

    template <class Pred>
    auto take_while_indexed(Pred&& pred) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while_indexed(Pred&& pred) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                                 static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct drop_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;
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

    auto drop(std::ptrdiff_t n) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    auto drop(std::ptrdiff_t n) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct take_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;

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

    auto take(std::ptrdiff_t n) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    auto take(std::ptrdiff_t n) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct step_mixin
{
    struct next_function
    {
        mutable std::ptrdiff_t m_count;
        next_function_t<T> m_next;
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

    auto step(std::ptrdiff_t n) const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<const sequence_t<T>&>(*this).get_next_function() } };
    }

    auto step(std::ptrdiff_t n) && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ n, static_cast<sequence_t<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct intersperse_mixin
{
    enum class state_t
    {
        start,
        yield_item,
        yield_separator
    };
    template <class E, class Out>
    struct next_function
    {
        next_function_t<T> m_next;
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

    template <class E, class Out = std::common_type_t<T, E>>
    auto intersperse(E element) const& -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<E, Out>{ static_cast<const sequence_t<T>&>(*this).get_next_function(),
                                                       std::move(element) } };
    }

    template <class E, class Out = std::common_type_t<T, E>>
    auto intersperse(E element) && -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<E, Out>{ static_cast<sequence_t<T>&&>(*this).get_next_function(),
                                                       std::move(element) } };
    }
};

template <class T>
struct join_mixin
{
};

template <class T>
struct join_mixin<sequence_t<T>>
{
    struct next_function
    {
        next_function_t<sequence_t<T>> m_next;
        mutable next_function_t<T> m_sub = {};

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                if (!m_sub)
                {
                    iteration_result_t<sequence_t<T>> next = m_next();
                    if (!next)
                    {
                        return {};
                    }
                    m_sub = next->get_next_function();
                    continue;
                }
                iteration_result_t<T> next_sub = m_sub();
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

    auto join() const& -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ static_cast<const sequence_t<sequence_t<T>>&>(*this).get_next_function() } };
    }

    auto join() && -> sequence_t<T>
    {
        return sequence_t<T>{ next_function{ static_cast<sequence_t<sequence_t<T>>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct for_each_mixin
{
    template <class Func>
    void for_each(Func&& func) const&
    {
        const auto next_function = static_cast<const sequence_t<T>&>(*this).get_next_function();
        while (true)
        {
            const iteration_result_t<T> next = next_function();
            if (!next)
            {
                break;
            }
            std::invoke(func, *next);
        }
    }
};

template <class T>
struct for_each_indexed_mixin
{
    template <class Func>
    void for_each_indexed(Func&& func) const&
    {
        const auto next_function = static_cast<const sequence_t<T>&>(*this).get_next_function();
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
    }
};

template <class T>
struct sequence_iterator
{
    using next_function_type = next_function_t<T>;
    using reference = T;
    using difference_type = std::ptrdiff_t;
    using value_type = std::decay_t<reference>;
    using pointer = std::conditional_t<  //
        std::is_reference_v<reference>,
        std::add_pointer_t<reference>,
        detail::pointer_proxy<reference>>;
    using iterator_category = std::input_iterator_tag;

    next_function_type m_next_fn;
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

template <class To, class From>
struct cast_sequence
{
    next_function_t<From> m_from;

    auto operator()() const -> iteration_result_t<To>
    {
        iteration_result_t<From> value = m_from();
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

template <class T>
struct is_sequence<sequence_t<T>> : std::true_type
{
};

}  // namespace detail

template <class T>
struct sequence_t : detail::inspect_mixin<T>,
                    detail::inspect_indexed_mixin<T>,
                    detail::transform_mixin<T>,
                    detail::transform_indexed_mixin<T>,
                    detail::filter_mixin<T>,
                    detail::filter_indexed_mixin<T>,
                    detail::transform_maybe_mixin<T>,
                    detail::transform_maybe_indexed_mixin<T>,
                    detail::drop_while_mixin<T>,
                    detail::drop_while_indexed_mixin<T>,
                    detail::take_while_mixin<T>,
                    detail::take_while_indexed_mixin<T>,
                    detail::drop_mixin<T>,
                    detail::take_mixin<T>,
                    detail::step_mixin<T>,
                    detail::intersperse_mixin<T>,
                    detail::join_mixin<T>,
                    detail::for_each_mixin<T>,
                    detail::for_each_indexed_mixin<T>
{
    using iterator = detail::sequence_iterator<T>;
    using next_function_type = typename iterator::next_function_type;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer = typename iterator::pointer;
    using difference_type = typename iterator::difference_type;

    next_function_type m_next_fn;

    explicit sequence_t(next_function_type next_fn) : m_next_fn(std::move(next_fn)) { }

    template <class U, std::enable_if_t<std::is_constructible_v<reference, U>, int> = 0>
    sequence_t(const sequence_t<U>& other) : sequence_t(detail::cast_sequence<reference, U>{ other.get_next_function() })
    {
    }

    template <class U, std::enable_if_t<std::is_constructible_v<reference, U>, int> = 0>
    sequence_t(sequence_t<U>&& other)
        : sequence_t(detail::cast_sequence<reference, U>{ std::move(other).get_next_function() })
    {
    }

    template <class Iter, std::enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>, int> = 0>
    explicit sequence_t(Iter b, Iter e) : sequence_t(detail::view_sequence<Iter, reference>{ b, e })
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        std::enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>, int> = 0>
    sequence_t(Range&& range) : sequence_t(std::begin(range), std::end(range))
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        std::enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>, int> = 0>
    sequence_t(Range range, int)
        : sequence_t(detail::owning_sequence<Range, Iter, reference>{ std::make_shared<Range>(std::move(range)) })
    {
    }

    sequence_t() : sequence_t(detail::empty_sequence<T>{}) { }

    template <
        class Container,
        std::enable_if_t<
            std::is_constructible_v<Container, iterator, iterator> && !detail::is_sequence<Container>::value,
            int> = 0>
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

    template <class Pred>
    auto find_if(Pred pred) const -> maybe_t<reference>
    {
        return this->drop_while(std::not_fn(std::move(pred))).maybe_front();
    }

    template <class Pred>
    auto index_of(Pred pred) const -> maybe_t<difference_type>
    {
        difference_type index = 0;
        iterator it = begin();
        const iterator e = end();
        for (; it != e; ++it, ++index)
        {
            if (std::invoke(pred, *it))
            {
                return index;
            }
        }
        return {};
    }

    template <class Func>
    auto transform_join(Func&& func) const -> decltype(this->transform(std::forward<Func>(func)).join())
    {
        return this->transform(std::forward<Func>(func)).join();
    }
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

    template <class T>
    auto operator()(T init) const -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<T>{ init } };
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

    template <class T>
    auto operator()(T lower, T upper) const -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<T>{ lower, upper } };
    }

    template <class T>
    auto operator()(T upper) const -> sequence_t<T>
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
        class Out = std::tuple_element_t<0, Res>>
    auto operator()(S state, Func&& func) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<std::decay_t<Func>, S, Out>{ std::forward<Func>(func), std::move(state) } };
    }
};

struct view_fn
{
    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range&& range) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ view_sequence<iterator_t<Range>, Out>{ std::begin(range), std::end(range) } };
    }

    template <class Iter, class Out = iter_reference_t<Iter>>
    auto operator()(Iter b, Iter e) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ view_sequence<Iter, Out>{ b, e } };
    }

    template <class T>
    auto operator()(const sequence_t<T>& seq) const -> sequence_t<T>
    {
        return seq;
    }

    template <class T>
    auto operator()(sequence_t<T>&& seq) const -> sequence_t<T>
    {
        return seq;
    }
};

struct owning_fn
{
    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range range) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ owning_sequence<Range, iterator_t<Range>, Out>{
            std::make_shared<Range>(std::move(range)) } };
    }

    template <class T>
    auto operator()(const sequence_t<T>& s) const -> sequence_t<T>
    {
        return s;
    }

    template <class T>
    auto operator()(sequence_t<T>&& s) const -> sequence_t<T>
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

    template <class T>
    auto operator()(T value) const -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<T>{ std::move(value) } };
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

    template <class T>
    auto operator()(T value) const -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<T>{ std::move(value) } };
    }
};

struct concat_fn
{
    template <class T>
    struct next_function
    {
        next_function_t<T> m_first;
        next_function_t<T> m_second;
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

    template <class T0, class T1, class T2, class T3, class Out = std::common_type_t<T0, T1, T2, T3>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1,
        const sequence_t<T2>& s2,
        const sequence_t<T3>& s3) const -> sequence_t<Out>
    {
        return (*this)((*this)(s0, s1, s2), s3);
    }

    template <class T0, class T1, class T2, class Out = std::common_type_t<T0, T1, T2>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1,
        const sequence_t<T2>& s2) const -> sequence_t<Out>
    {
        return (*this)((*this)(s0, s1), s2);
    }

    template <class T0, class T1, class Out = std::common_type_t<T0, T1>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1) const -> sequence_t<Out>
    {
        return (*this)(sequence_t<Out>{ s0 }, sequence_t<Out>{ s1 });
    }

    template <class T>
    auto operator()(  //
        const sequence_t<T>& lhs,
        const sequence_t<T>& rhs) const -> sequence_t<T>
    {
        return sequence_t<T>{ next_function<T>{ lhs.get_next_function(), rhs.get_next_function() } };
    }
};

struct zip_fn
{
    template <class In0, class In1 = void, class In2 = void, class In3 = void>
    struct next_function;

    template <class In0, class In1, class In2, class In3>
    struct next_function
    {
        next_function_t<In0> m_next0;
        next_function_t<In1> m_next1;
        next_function_t<In2> m_next2;
        next_function_t<In3> m_next3;

        auto operator()() const -> iteration_result_t<std::tuple<In0, In1, In2, In3>>
        {
            iteration_result_t<In0> n0 = m_next0();
            iteration_result_t<In1> n1 = m_next1();
            iteration_result_t<In2> n2 = m_next2();
            iteration_result_t<In3> n3 = m_next3();
            if (n0 && n1 && n2 && n3)
            {
                return std::tuple<In0, In1, In2, In3>{ *n0, *n1, *n2, *n3 };
            }
            return {};
        }
    };

    template <class In0, class In1, class In2>
    struct next_function<In0, In1, In2, void>
    {
        next_function_t<In0> m_next0;
        next_function_t<In1> m_next1;
        next_function_t<In2> m_next2;

        auto operator()() const -> iteration_result_t<std::tuple<In0, In1, In2>>
        {
            iteration_result_t<In0> n0 = m_next0();
            iteration_result_t<In1> n1 = m_next1();
            iteration_result_t<In2> n2 = m_next2();
            if (n0 && n1 && n2)
            {
                return std::tuple<In0, In1, In2>{ *n0, *n1, *n2 };
            }
            return {};
        }
    };

    template <class In0, class In1>
    struct next_function<In0, In1, void, void>
    {
        next_function_t<In0> m_next0;
        next_function_t<In1> m_next1;

        auto operator()() const -> iteration_result_t<std::tuple<In0, In1>>
        {
            iteration_result_t<In0> n0 = m_next0();
            iteration_result_t<In1> n1 = m_next1();
            if (n0 && n1)
            {
                return std::tuple<In0, In1>{ *n0, *n1 };
            }
            return {};
        }
    };

    template <class T0, class T1, class T2, class T3, class Out = std::tuple<T0, T1, T2, T3>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1,
        const sequence_t<T2>& s2,
        const sequence_t<T3>& s3) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<T0, T1, T2, T3>{
            s0.get_next_function(), s1.get_next_function(), s2.get_next_function(), s3.get_next_function() } };
    }

    template <class T0, class T1, class T2, class Out = std::tuple<T0, T1, T2>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1,
        const sequence_t<T2>& s2) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<T0, T1, T2>{
            s0.get_next_function(), s1.get_next_function(), s2.get_next_function() } };
    }

    template <class T0, class T1, class Out = std::tuple<T0, T1>>
    auto operator()(  //
        const sequence_t<T0>& s0,
        const sequence_t<T1>& s1) const -> sequence_t<Out>
    {
        return sequence_t<Out>{ next_function<T0, T1>{ s0.get_next_function(), s1.get_next_function() } };
    }
};

struct vec_fn
{
    template <class T, class... Tail>
    auto operator()(T head, Tail&&... tail) const -> sequence_t<const T&>
    {
        return owning_fn{}(std::vector<T>{ std::move(head), std::forward<Tail>(tail)... });
    }
};

struct init_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(std::ptrdiff_t n, Func&& func) const -> sequence_t<Out>
    {
        return range_fn{}(n).transform(std::forward<Func>(func));
    }
};

struct init_infinite_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(Func&& func) const -> sequence_t<Out>
    {
        return iota_fn{}(std::ptrdiff_t{ 0 }).transform(std::forward<Func>(func));
    }
};

}  // namespace detail

namespace seq
{

static constexpr inline auto iota = detail::iota_fn{};
static constexpr inline auto range = detail::range_fn{};
static constexpr inline auto unfold = detail::unfold_fn{};
static constexpr inline auto view = detail::view_fn{};
static constexpr inline auto owning = detail::owning_fn{};
static constexpr inline auto repeat = detail::repeat_fn{};
static constexpr inline auto single = detail::single_fn{};
static constexpr inline auto concat = detail::concat_fn{};
static constexpr inline auto vec = detail::vec_fn{};
static constexpr inline auto zip = detail::zip_fn{};
static constexpr inline auto init = detail::init_fn{};
static constexpr inline auto init_infinite = detail::init_infinite_fn{};

}  // namespace seq

template <class L, class R>
auto operator+(const sequence_t<L>& lhs, const sequence_t<R>& rhs) -> sequence_t<std::common_type_t<L, R>>
{
    return concat(lhs, rhs);
}

}  // namespace zx
