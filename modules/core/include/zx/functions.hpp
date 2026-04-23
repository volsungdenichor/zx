#pragma once

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
    template <class T, std::enable_if_t<!is_maybe_dereferenceable<T>::value, int> = 0>
    constexpr auto operator()(T& item) const -> T&
    {
        return item;
    }

    template <class T, std::enable_if_t<is_maybe_dereferenceable<T>::value, int> = 0>
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

    template <class Func, class Proj>
    constexpr auto operator()(Func&& func, Proj&& proj) const
    {
        return impl_t<std::decay_t<Func>, std::decay_t<Proj>>{ std::forward<Func>(func), std::forward<Proj>(proj) };
    }
};

}  // namespace detail

template <std::size_t N>
constexpr auto get_element = detail::get_element_fn<N>{};

constexpr inline auto get_key = get_element<0>;
constexpr inline auto get_value = get_element<1>;

constexpr inline auto get_first = get_element<0>;
constexpr inline auto get_second = get_element<1>;

static constexpr inline auto dereference = detail::dereference_fn{};
static constexpr inline auto proj = detail::proj_fn{};

}  // namespace zx
