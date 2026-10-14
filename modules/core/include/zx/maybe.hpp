#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <utility>
#include <zx/pipe.hpp>
#include <zx/type_traits.hpp>

namespace zx
{

template <class T>
struct maybe_t;

template <class T>
struct maybe_underlying_type;

template <class T>
using maybe_underlying_type_t = typename maybe_underlying_type<T>::type;

template <class T>
struct maybe_underlying_type<maybe_t<T>>
{
    using type = T;
};

struct bad_maybe_access : std::exception
{
    const char* what() const noexcept override { return "bad_maybe_access"; }
};

namespace detail
{

template <class T>
struct is_maybe : std::false_type
{
};

template <class T>
struct is_maybe<maybe_t<T>> : std::true_type
{
};

}  // namespace detail

struct none_t
{
};

static constexpr none_t none{};

template <class T>
struct maybe_t
{
    static_assert(!std::is_rvalue_reference_v<T>, "maybe_t element type must not be an rvalue reference");

    using value_type = T;

    constexpr maybe_t() : m_storage() { }

    constexpr maybe_t(none_t) : maybe_t() { }

    constexpr maybe_t(value_type value) : m_storage(std::move(value)) { }

    template <class U>
    constexpr maybe_t(const maybe_t<U>& other) : maybe_t()
    {
        if (other)
        {
            m_storage = *other;
        }
    }

    template <class U>
    constexpr maybe_t(maybe_t<U>&& other) : maybe_t()
    {
        if (other)
        {
            m_storage = *std::move(other);
        }
    }

    constexpr maybe_t(const maybe_t&) = default;
    constexpr maybe_t(maybe_t&&) noexcept = default;

    maybe_t& operator=(maybe_t other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const { return m_storage.has_value(); }

    constexpr const value_type& get() const& { return *m_storage; }

    constexpr value_type& get() & { return *m_storage; }

    constexpr value_type&& get() && { return *std::move(m_storage); }

    constexpr const value_type& operator*() const&
    {
        if (!has_value())
        {
            throw bad_maybe_access{};
        }
        return get();
    }

    constexpr value_type& operator*() &
    {
        if (!has_value())
        {
            throw bad_maybe_access{};
        }
        return get();
    }

    constexpr value_type&& operator*() &&
    {
        if (!has_value())
        {
            throw bad_maybe_access{};
        }
        return std::move(*this).get();
    }

    constexpr const value_type* operator->() const& { return &**this; }

    constexpr value_type* operator->() & { return &**this; }

    constexpr const value_type& value() const& { return **this; }

    constexpr value_type& value() & { return **this; }

    constexpr value_type&& value() && { return *std::move(*this); }

    constexpr bool has_value() const noexcept { return static_cast<bool>(*this); }

private:
    std::optional<value_type> m_storage;
};

template <class T>
struct maybe_t<T&>
{
    using value_type = T;

    constexpr maybe_t() : m_storage() { }

    constexpr maybe_t(none_t) : maybe_t() { }

    constexpr maybe_t(value_type& value) : m_storage(&value) { }

    constexpr maybe_t(const maybe_t&) = default;
    constexpr maybe_t(maybe_t&&) noexcept = default;

    maybe_t& operator=(maybe_t other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const { return static_cast<bool>(m_storage); }

    constexpr value_type& get() const { return *m_storage; }

    constexpr value_type& operator*() const
    {
        if (!has_value())
        {
            throw bad_maybe_access{};
        }
        return get();
    }

    constexpr value_type* operator->() const { return &**this; }

    constexpr value_type& value() const { return **this; }

    constexpr bool has_value() const noexcept { return static_cast<bool>(*this); }

private:
    value_type* m_storage;
};

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const maybe_t<L>& lhs, const maybe_t<R>& rhs)
{
    if (lhs.has_value() != rhs.has_value())
    {
        return false;
    }
    if (!lhs.has_value())
    {
        return true;
    }
    return *lhs == *rhs;
}

template <class L, class R>
constexpr bool operator!=(const maybe_t<L>& lhs, const maybe_t<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const maybe_t<L>& lhs, const R& rhs)
{
    return lhs && *lhs == rhs;
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const maybe_t<L>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const L& lhs, const maybe_t<R>& rhs)
{
    return rhs && lhs == *rhs;
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const L& lhs, const maybe_t<R>& rhs)
{
    return !(lhs == rhs);
}

template <class T>
constexpr bool operator==(const maybe_t<T>& m, none_t)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(const maybe_t<T>& m, none_t)
{
    return m.has_value();
}

template <class T>
constexpr bool operator==(none_t, const maybe_t<T>& m)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(none_t, const maybe_t<T>& m)
{
    return m.has_value();
}

template <class T>
constexpr bool operator==(const maybe_t<T>& m, std::nullopt_t)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(const maybe_t<T>& m, std::nullopt_t)
{
    return m.has_value();
}

template <class T>
constexpr bool operator==(std::nullopt_t, const maybe_t<T>& m)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(std::nullopt_t, const maybe_t<T>& m)
{
    return m.has_value();
}

template <class T, enable_if_t<has_ostream_operator<T>::value> = 0>
std::ostream& operator<<(std::ostream& os, const maybe_t<T>& item)
{
    if (item)
    {
        os << "maybe(" << *item << ")";
    }
    else
    {
        os << "none";
    }
    return os;
}

namespace maybe
{

namespace detail
{

struct transform_fn
{
    template <class Func>
    struct impl_t
    {
        Func m_func;

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const T&>>,
            class Result = maybe_t<FuncResult>>
        constexpr auto operator()(const maybe_t<T>& value) const -> Result
        {
            return value ? Result{ std::invoke(m_func, value.get()) } : Result{};
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = maybe_t<FuncResult>>
        constexpr auto operator()(maybe_t<T>&& value) const -> Result
        {
            return value ? Result{ std::invoke(m_func, std::move(value).get()) } : Result{};
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&>>,
            class Result = maybe_t<FuncResult>>
        constexpr auto operator()(maybe_t<T&> value) const -> Result
        {
            return value ? Result{ std::invoke(m_func, value.get()) } : Result{};
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ std::forward<Func>(func) });
    }
};

struct and_then_fn
{
    template <class Func>
    struct impl_t
    {
        Func m_func;

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const T&>>,
            class Result = FuncResult>
        constexpr auto operator()(const maybe_t<T>& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_maybe<FuncResult>::value,
                "and_then: function result type needs to be of `maybe_t<T>` type");
            return value ? Result{ std::invoke(m_func, value.get()) } : Result{};
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = FuncResult>
        constexpr auto operator()(maybe_t<T>&& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_maybe<FuncResult>::value,
                "and_then: function result type needs to be of `maybe_t<T>` type");
            return value ? Result{ std::invoke(m_func, std::move(value).get()) } : Result{};
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&>>,
            class Result = FuncResult>
        constexpr auto operator()(maybe_t<T&> value) const -> Result
        {
            static_assert(
                ::zx::detail::is_maybe<FuncResult>::value,
                "and_then: function result type needs to be of `maybe_t<T>` type");
            return value ? Result{ std::invoke(m_func, value.get()) } : Result{};
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ std::forward<Func>(func) });
    }
};

struct filter_fn
{
    template <class Pred>
    struct impl_t
    {
        Pred m_pred;

        template <class T, class Result = maybe_t<T>>
        constexpr auto operator()(const maybe_t<T>& value) const -> Result
        {
            return value && std::invoke(m_pred, value.get()) ? Result{ value.get() } : Result{};
        }

        template <class T, class Result = maybe_t<T>>
        constexpr auto operator()(maybe_t<T>&& value) const -> Result
        {
            return value && std::invoke(m_pred, value.get()) ? Result{ std::move(value).get() } : Result{};
        }

        template <class T, class Result = maybe_t<T&>>
        constexpr auto operator()(maybe_t<T&> value) const -> Result
        {
            return value && std::invoke(m_pred, value.get()) ? Result{ value.get() } : Result{};
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const
    {
        return fn(impl_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) });
    }
};

struct or_else_fn
{
    template <class Func>
    struct impl_t
    {
        Func m_func;

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe_t<T>, FuncResult>>
        constexpr auto operator()(const maybe_t<T>& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                if (!value)
                {
                    std::invoke(m_func);
                }
                return Result{ value };
            }
            else
            {
                static_assert(
                    ::zx::detail::is_maybe<FuncResult>::value,
                    "or_else: function result type needs to be of `maybe_t<T>` type");
                return value ? Result{ value } : Result{ std::invoke(m_func) };
            }
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe_t<T>, FuncResult>>
        constexpr auto operator()(maybe_t<T>&& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                if (!value)
                {
                    std::invoke(m_func);
                }
                return Result{ std::move(value) };
            }
            else
            {
                static_assert(
                    ::zx::detail::is_maybe<FuncResult>::value,
                    "or_else: function result type needs to be of `maybe_t<T>` type");
                return value ? Result{ std::move(value) } : Result{ std::invoke(m_func) };
            }
        }

        template <
            class T,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe_t<T&>, FuncResult>>
        constexpr auto operator()(maybe_t<T&> value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                if (!value)
                {
                    std::invoke(m_func);
                }
                return Result{ value };
            }
            else
            {
                static_assert(
                    ::zx::detail::is_maybe<FuncResult>::value,
                    "or_else: function result type needs to be of `maybe_t<T>` type");
                return value ? Result{ value } : Result{ std::invoke(m_func) };
            }
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ std::forward<Func>(func) });
    }
};

struct value_or_fn
{
    template <class U>
    struct impl_t
    {
        U m_default_value;

        template <class T>
        constexpr auto operator()(const maybe_t<T>& value) const -> T
        {
            return value ? value.get() : static_cast<T>(m_default_value);
        }

        template <class T>
        constexpr auto operator()(maybe_t<T>&& value) const -> T
        {
            return value ? std::move(value).get() : static_cast<T>(m_default_value);
        }

        template <class T>
        constexpr auto operator()(maybe_t<T&> value) const -> T
        {
            return value ? value.get() : static_cast<T>(m_default_value);
        }
    };

    template <class U>
    constexpr auto operator()(U&& default_value) const
    {
        return fn(impl_t<std::decay_t<U>>{ std::forward<U>(default_value) });
    }
};

}  // namespace detail

constexpr inline auto transform = detail::transform_fn{};
constexpr inline auto and_then = detail::and_then_fn{};
constexpr inline auto filter = detail::filter_fn{};
constexpr inline auto or_else = detail::or_else_fn{};
constexpr inline auto value_or = detail::value_or_fn{};

}  // namespace maybe

}  // namespace zx
