#pragma once

#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <zx/type_traits.hpp>

namespace zx
{

struct identity_t
{
    template <class T>
    constexpr T&& operator()(T&& value) const noexcept
    {
        return std::forward<T>(value);
    }
};

namespace detail
{

template <class T>
using dereference_result_t = decltype(*std::declval<T&>());

template <class T>
struct is_dereferenceable : is_detected<dereference_result_t, T>
{
};

template <class T>
struct is_maybe_dereferenceable : std::conjunction<std::is_constructible<bool, T&>, is_dereferenceable<T>>
{
};

template <std::size_t N>
struct get_element_fn
{
    template <class T>
    constexpr auto operator()(T&& tuple) const -> decltype(std::get<N>(std::forward<T>(tuple)))
    {
        return std::get<N>(std::forward<T>(tuple));
    }
};

struct dereference_fn
{
    template <class T, enable_if_t<!is_maybe_dereferenceable<T>::value> = 0>
    constexpr auto operator()(T& item) const -> T&
    {
        return item;
    }

    template <class T, enable_if_t<is_maybe_dereferenceable<T>::value> = 0>
    constexpr auto operator()(T&& item) const -> decltype((*this)(*std::forward<T>(item)))
    {
        if (!static_cast<bool>(item))
        {
            throw std::runtime_error{ "Attempted to dereference an empty value" };
        }
        return (*this)(*std::forward<T>(item));
    }

    template <class T>
    constexpr auto operator()(std::reference_wrapper<T> item) const -> T&
    {
        return (*this)(item.get());
    }
};

struct proj_fn
{
    template <class Func, class Proj>
    struct impl_t
    {
        Func m_func;
        Proj m_proj;

        template <class... Args>
        constexpr auto operator()(Args&&... args) const
        {
            return std::invoke(m_func, std::invoke(m_proj, std::forward<Args>(args))...);
        }
    };

public:
    template <class Func, class Proj>
    constexpr auto operator()(Func&& func, Proj&& proj) const
    {
        return impl_t<std::decay_t<Func>, std::decay_t<Proj>>{ std::forward<Func>(func), std::forward<Proj>(proj) };
    }
};

template <class T>
struct cast_fn
{
    template <class U>
    constexpr T operator()(U&& value) const
    {
        return static_cast<T>(std::forward<U>(value));
    }
};

struct overloaded_fn
{
private:
    template <class T>
    struct exact_arg_t
    {
        T value;

        constexpr operator T() && noexcept { return std::forward<T>(value); }
    };

    template <class... Funcs>
    struct overload_set_t : Funcs...
    {
        using Funcs::operator()...;

        constexpr explicit overload_set_t(Funcs... funcs) : Funcs(std::move(funcs))... { }
    };

    template <class... Funcs>
    struct impl_t
    {
        overload_set_t<Funcs...> m_funcs;

        template <class... Args>
        auto operator()(Args&&... args) const -> decltype(auto)
        {
            if constexpr (std::is_invocable_v<overload_set_t<Funcs...>, const impl_t&, exact_arg_t<Args&&>...>)
            {
                return std::invoke(m_funcs, *this, exact_arg_t<Args&&>{ std::forward<Args>(args) }...);
            }
            else
            {
                return call(*this, std::forward<Args>(args)...);
            }
        }

        template <std::size_t I = 0, class... Args>
        auto call(const impl_t& self, Args&&... args) const -> decltype(auto)
        {
            if constexpr (I == sizeof...(Funcs))
            {
                static_assert(always_false<Args...>::value, "No overloaded function matches the given arguments");
            }
            else
            {
                using Func = std::tuple_element_t<I, std::tuple<Funcs...>>;
                if constexpr (std::is_invocable_v<Func, const impl_t&, Args...>)
                {
                    return std::invoke(static_cast<const Func&>(self.m_funcs), self, std::forward<Args>(args)...);
                }
                else
                {
                    return call<I + 1>(self, std::forward<Args>(args)...);
                }
            }
        }
    };

public:
    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const
    {
        return impl_t<std::decay_t<Funcs>...>{ overload_set_t<std::decay_t<Funcs>...>{ std::forward<Funcs>(funcs)... } };
    }
};

}  // namespace detail

template <std::size_t N>
constexpr auto get_element = detail::get_element_fn<N>{};

inline constexpr auto get_key = get_element<0>;
inline constexpr auto get_value = get_element<1>;

inline constexpr auto get_first = get_element<0>;
inline constexpr auto get_second = get_element<1>;

inline constexpr auto dereference = detail::dereference_fn{};
inline constexpr auto proj = detail::proj_fn{};

template <class T>
inline constexpr auto cast = detail::cast_fn<T>{};

inline constexpr auto overloaded = detail::overloaded_fn{};

}  // namespace zx
