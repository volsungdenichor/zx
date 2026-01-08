#pragma once

#include <zx/type_traits.hpp>

namespace zx
{

namespace detail
{

template <template <class> class C>
struct convertible_to
{
    template <class T, class = std::enable_if_t<C<T>::value>>
    operator T() const;
};

template <class T>
using iter_has_deref_impl = decltype(std::declval<T>().deref());

template <class T>
using iter_has_inc_impl = decltype(std::declval<T>().inc());

template <class T>
using iter_has_dec_impl = decltype(std::declval<T>().dec());

template <class T>
using iter_has_advance_impl = decltype(std::declval<T>().advance(std::declval<convertible_to<std::is_integral>>()));

template <class T>
using iter_has_is_equal_impl = decltype(std::declval<T>().is_equal(std::declval<T>()));

template <class T>
using iter_has_is_less_impl = decltype(std::declval<T>().is_less(std::declval<T>()));

template <class T>
using iter_has_distance_to_impl = decltype(std::declval<T>().distance_to(std::declval<T>()));

template <class T>
struct iter_has_deref : is_detected<iter_has_deref_impl, T>
{
};

template <class T>
struct iter_has_inc : is_detected<iter_has_inc_impl, T>
{
};

template <class T>
struct iter_has_dec : is_detected<iter_has_dec_impl, T>
{
};

template <class T>
struct iter_has_advance : is_detected<iter_has_advance_impl, T>
{
};

template <class T>
struct iter_has_is_equal : is_detected<iter_has_is_equal_impl, T>
{
};

template <class T>
struct iter_has_is_less : is_detected<iter_has_is_less_impl, T>
{
};

template <class T>
struct iter_has_distance_to : is_detected<iter_has_distance_to_impl, T>
{
};

template <class T, class = std::void_t<>>
struct difference_type_impl
{
    using type = std::ptrdiff_t;
};

template <class T>
struct difference_type_impl<T, std::enable_if_t<iter_has_distance_to<T>::value>>
{
    using type = decltype(std::declval<T>().distance_to(std::declval<T>()));
};

template <class T>
struct iter_is_incrementable : std::bool_constant<iter_has_inc<T>::value || iter_has_advance<T>::value>
{
};

template <class T>
struct iter_is_decrementable : std::bool_constant<iter_has_advance<T>::value || iter_has_dec<T>::value>
{
};

template <class T>
struct iter_is_equality_comparable : std::bool_constant<iter_has_is_equal<T>::value || iter_has_distance_to<T>::value>
{
};

template <class T>
struct iter_is_less_than_comparable : std::bool_constant<iter_has_is_less<T>::value || iter_has_distance_to<T>::value>
{
};

template <class T>
using difference_type = typename detail::difference_type_impl<T>::type;

template <class T>
struct pointer_proxy
{
    T item;

    constexpr T* operator->()
    {
        return std::addressof(item);
    }
};

template <class Impl>
struct iterator_interface
{
    Impl m_impl;

    template <class... Args, std::enable_if_t<std::is_constructible<Impl, Args...>::value, int> = 0>
    constexpr iterator_interface(Args&&... args) : m_impl{ std::forward<Args>(args)... }
    {
    }

    constexpr iterator_interface() = default;
    constexpr iterator_interface(const iterator_interface&) = default;
    constexpr iterator_interface(iterator_interface&&) noexcept = default;

    constexpr iterator_interface& operator=(iterator_interface other)
    {
        std::swap(m_impl, other.m_impl);
        return *this;
    }

    static_assert(iter_has_deref<Impl>::value, "iterator_interface: .deref required");
    static_assert(iter_is_equality_comparable<Impl>::value, "iterator_interface: either .is_equal or .distance_to required");
    static_assert(
        std::is_default_constructible<Impl>::value, "iterator_interface: default constructible implementation required");

    using reference = decltype(m_impl.deref());

private:
    template <class R = reference, std::enable_if_t<std::is_reference<R>::value, int> = 0>
    constexpr auto get_pointer() const -> std::add_pointer_t<reference>
    {
        return std::addressof(**this);
    }

    template <class R = reference, std::enable_if_t<!std::is_reference<R>::value, int> = 0>
    constexpr auto get_pointer() const -> pointer_proxy<reference>
    {
        return { **this };
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_incrementable<Impl_>::value, int> = 0>
    constexpr void inc()
    {
        if constexpr (iter_has_inc<Impl_>::value)
        {
            m_impl.inc();
        }
        else
        {
            m_impl.advance(+1);
        }
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_decrementable<Impl_>::value, int> = 0>
    constexpr void dec()
    {
        if constexpr (iter_has_dec<Impl_>::value)
        {
            m_impl.dec();
        }
        else
        {
            m_impl.advance(-1);
        }
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_equality_comparable<Impl_>::value, int> = 0>
    constexpr bool is_equal(const Impl& other) const
    {
        if constexpr (iter_has_is_equal<Impl_>::value)
        {
            return m_impl.is_equal(other);
        }
        else
        {
            return m_impl.distance_to(other) == 0;
        }
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_less_than_comparable<Impl_>::value, int> = 0>
    constexpr bool is_less(const Impl& other) const
    {
        if constexpr (iter_has_is_less<Impl_>::value)
        {
            return m_impl.is_less(other);
        }
        else
        {
            return m_impl.distance_to(other) > 0;
        }
    }

public:
    constexpr reference operator*() const
    {
        return m_impl.deref();
    }

    constexpr auto operator->() const -> decltype(get_pointer())
    {
        return get_pointer();
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_incrementable<Impl_>::value, int> = 0>
    iterator_interface& operator++()
    {
        inc();
        return *this;
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_incrementable<Impl_>::value, int> = 0>
    iterator_interface operator++(int)
    {
        iterator_interface tmp{ *this };
        ++(*this);
        return tmp;
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_decrementable<Impl_>::value, int> = 0>
    iterator_interface& operator--()
    {
        dec();
        return *this;
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_decrementable<Impl_>::value, int> = 0>
    iterator_interface operator--(int)
    {
        iterator_interface tmp{ *this };
        --(*this);
        return tmp;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    friend iterator_interface& operator+=(iterator_interface& it, D offset)
    {
        it.m_impl.advance(offset);
        return it;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    friend iterator_interface operator+(iterator_interface it, D offset)
    {
        return it += offset;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    friend iterator_interface operator+(D offset, iterator_interface it)
    {
        return it + offset;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    friend iterator_interface& operator-=(iterator_interface& it, D offset)
    {
        return it += -offset;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    friend iterator_interface operator-(iterator_interface it, D offset)
    {
        return it -= offset;
    }

    template <
        class D,
        class Impl_ = Impl,
        std::enable_if_t<iter_has_advance<Impl_>::value && std::is_integral_v<D>, int> = 0>
    reference operator[](D offset) const
    {
        return *(*this + offset);
    }

    friend bool operator==(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return lhs.is_equal(rhs.m_impl);
    }

    friend bool operator!=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs == rhs);
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_less_than_comparable<Impl_>::value, int> = 0>
    friend bool operator<(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return lhs.is_less(rhs.m_impl);
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_less_than_comparable<Impl_>::value, int> = 0>
    friend bool operator>(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return rhs < lhs;
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_less_than_comparable<Impl_>::value, int> = 0>
    friend bool operator<=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs > rhs);
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_is_less_than_comparable<Impl_>::value, int> = 0>
    friend bool operator>=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs < rhs);
    }

    template <class Impl_ = Impl, std::enable_if_t<iter_has_distance_to<Impl_>::value, int> = 0>
    friend auto operator-(const iterator_interface& lhs, const iterator_interface& rhs) -> difference_type<Impl_>
    {
        return rhs.m_impl.distance_to(lhs.m_impl);
    }
};

template <class T, class = std::void_t<>>
struct iterator_category_impl
{
    using type = std::conditional_t<
        iter_has_advance<T>::value && iter_has_distance_to<T>::value,
        std::random_access_iterator_tag,
        std::conditional_t<
            iter_has_dec<T>::value || iter_has_advance<T>::value,
            std::bidirectional_iterator_tag,
            std::forward_iterator_tag>>;
};

template <class T>
struct iterator_category_impl<T, std::void_t<typename T::iterator_category>>
{
    using type = typename T::iterator_category;
};

}  // namespace detail

using detail::iterator_interface;

}  // namespace zx

namespace std
{
template <class Impl>
struct iterator_traits<::zx::iterator_interface<Impl>>
{
    using it = ::zx::iterator_interface<Impl>;
    using reference = decltype(std::declval<it>().operator*());
    using pointer = decltype(std::declval<it>().operator->());
    using value_type = std::decay_t<reference>;
    using difference_type = typename ::zx::detail::difference_type<Impl>;
    using iterator_category = typename ::zx::detail::iterator_category_impl<Impl>::type;
};

}  // namespace std
