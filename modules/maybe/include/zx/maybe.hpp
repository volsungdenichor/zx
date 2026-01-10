#pragma once

#include <exception>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace zx
{

template <class T>
struct maybe;

template <class T>
struct maybe_underlying_type;

template <class T>
using maybe_underlying_type_t = typename maybe_underlying_type<T>::type;

template <class T>
struct maybe_underlying_type<maybe<T>>
{
    using type = T;
};

struct bad_maybe_access : std::exception
{
    const char* what() const noexcept override
    {
        return "zx::bad_maybe_access";
    }
};

namespace detail
{

template <class T>
struct is_maybe : std::false_type
{
};

template <class T>
struct is_maybe<maybe<T>> : std::true_type
{
};

}  // namespace detail

struct none_t
{
};

static constexpr none_t none{};

template <class T>
struct maybe
{
    using value_type = T;

    constexpr maybe() : m_storage()
    {
    }

    constexpr maybe(none_t) : maybe()
    {
    }

    constexpr maybe(value_type value) : m_storage(std::move(value))
    {
    }

    template <class U>
    constexpr maybe(const maybe<U>& other) : maybe()
    {
        if (other)
        {
            m_storage = *other;
        }
    }

    template <class U>
    constexpr maybe(maybe<U>&& other) : maybe()
    {
        if (other)
        {
            m_storage = *std::move(other);
        }
    }

    constexpr maybe(const maybe&) = default;
    constexpr maybe(maybe&&) noexcept = default;

    maybe& operator=(maybe other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return m_storage.has_value();
    }

    constexpr const value_type& get() const&
    {
        return *m_storage;
    }

    constexpr value_type& get() &
    {
        return *m_storage;
    }

    constexpr value_type&& get() &&
    {
        return *std::move(m_storage);
    }

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

    constexpr const value_type* operator->() const&
    {
        return &**this;
    }

    constexpr value_type* operator->() &
    {
        return &**this;
    }

    constexpr const value_type& value() const&
    {
        return **this;
    }

    constexpr value_type& value() &
    {
        return **this;
    }

    constexpr value_type&& value() &&
    {
        return *std::move(*this);
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), get()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).get()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), get()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).get()) }
                   : Result{};
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func)), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func)), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) const& -> maybe<T>
    {
        return *this && std::invoke(std::forward<Pred>(pred), get())  //
                   ? maybe<T>{ *this }
                   : maybe<T>{};
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) && -> maybe<T>
    {
        return *this && std::invoke(std::forward<Pred>(pred), get())  //
                   ? maybe<T>{ std::move(*this) }
                   : maybe<T>{};
    }

    template <class U>
    constexpr auto value_or(U&& v) const& -> value_type
    {
        return *this ? get() : static_cast<value_type>(std::forward<U>(v));
    }

    template <class U>
    constexpr auto value_or(U&& v) && -> value_type
    {
        return *this ? std::move(*this).get() : static_cast<value_type>(std::forward<U>(v));
    }

private:
    std::optional<value_type> m_storage;
};

template <class T>
struct maybe<T&>
{
    using value_type = T;

    constexpr maybe() : m_storage()
    {
    }

    constexpr maybe(none_t) : maybe()
    {
    }

    constexpr maybe(value_type& value) : m_storage(&value)
    {
    }

    constexpr maybe(const maybe&) = default;
    constexpr maybe(maybe&&) noexcept = default;

    maybe& operator=(maybe other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return static_cast<bool>(m_storage);
    }

    constexpr value_type& get() const
    {
        return *m_storage;
    }

    constexpr value_type& operator*() const
    {
        if (!has_value())
        {
            throw bad_maybe_access{};
        }
        return get();
    }

    constexpr value_type* operator->() const
    {
        return &**this;
    }

    constexpr value_type& value() const
    {
        return **this;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), get()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), get()) }
                   : Result{};
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, maybe<T>, FuncResult>>
    constexpr auto or_else(Func&& func) const -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func)), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_maybe<FuncResult>::value, "or_else: function result type needs to be of `maybe<T>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func)) };
        }
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) const -> maybe<T&>
    {
        return *this && std::invoke(std::forward<Pred>(pred), get())  //
                   ? maybe<T&>{ *this }
                   : maybe<T&>{};
    }

    template <class U>
    constexpr auto value_or(U&& v) const -> value_type
    {
        return *this ? get() : static_cast<value_type>(std::forward<U>(v));
    }

private:
    value_type* m_storage;
};

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const maybe<L>& lhs, const maybe<R>& rhs)
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
constexpr bool operator!=(const maybe<L>& lhs, const maybe<R>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const maybe<L>& lhs, const R& rhs)
{
    return lhs && *lhs == rhs;
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const maybe<L>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const L& lhs, const maybe<R>& rhs)
{
    return rhs && lhs == *rhs;
}

template <class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const L& lhs, const maybe<R>& rhs)
{
    return !(lhs == rhs);
}

template <class T>
constexpr bool operator==(const maybe<T>& m, none_t)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(const maybe<T>& m, none_t)
{
    return m.has_value();
}

template <class T>
constexpr bool operator==(none_t, const maybe<T>& m)
{
    return !m.has_value();
}

template <class T>
constexpr bool operator!=(none_t, const maybe<T>& m)
{
    return m.has_value();
}

}  // namespace zx
