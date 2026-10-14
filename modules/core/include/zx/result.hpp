#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <zx/pipe.hpp>
#include <zx/type_traits.hpp>

namespace zx
{

template <class T, class E>
struct result_t;

namespace detail
{

template <class T>
struct is_result : std::false_type
{
};

template <class T, class E>
struct is_result<result_t<T, E>> : std::true_type
{
};

template <class E>
struct error_wrapper_t
{
    E m_error;
};

template <class E>
constexpr auto error(E&& error) -> error_wrapper_t<std::decay_t<E>>
{
    return error_wrapper_t<std::decay_t<E>>{ std::forward<E>(error) };
};

template <class Result>
struct to_result
{
    template <class Func, class... Args>
    constexpr auto operator()(Func&& func, Args&&... args) const -> Result
    {
        return Result{ std::invoke(std::forward<Func>(func), std::forward<Args>(args)...) };
    }
};

template <class E>
struct to_result<result_t<void, E>>
{
    template <class Func, class... Args>
    constexpr auto operator()(Func&& func, Args&&... args) const -> result_t<void, E>
    {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        return result_t<void, E>{};
    }
};

template <class Result, class Func, class... Args>
constexpr auto to_ok(Func&& func, Args&&... args) -> Result
{
    return to_result<Result>{}(std::forward<Func>(func), std::forward<Args>(args)...);
}

template <class Result, class In>
constexpr auto to_error(In&& in) -> Result
{
    return Result{ error(std::forward<In>(in).error()) };
}

}  // namespace detail

using detail::error;
using detail::error_wrapper_t;

struct bad_result_access : std::runtime_error
{
    explicit bad_result_access(const std::string& msg) : std::runtime_error{ msg } { }
};

template <class T, class E>
struct result_t
{
    static_assert(!std::is_rvalue_reference_v<T>, "result_t element type must not be an rvalue reference");

    using value_type = T;
    using error_type = E;
    using value_storage = value_type;
    using error_storage = error_wrapper_t<error_type>;

    constexpr result_t(value_type value = {}) : m_storage(std::in_place_type<value_storage>, std::move(value)) { }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(const error_wrapper_t<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(error_wrapper_t<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result_t(const result_t&) = default;
    constexpr result_t(result_t&&) noexcept = default;

    constexpr result_t& operator=(result_t other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const { return std::holds_alternative<value_storage>(m_storage); }

    constexpr const value_type& get() const& { return std::get<value_storage>(m_storage); }

    constexpr value_type& get() & { return std::get<value_storage>(m_storage); }

    constexpr value_type&& get() && { return std::get<value_storage>(std::move(m_storage)); }

    constexpr const value_type& operator*() const&
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result_t' object" };
        }
        return get();
    }

    constexpr value_type& operator*() &
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result_t' object" };
        }
        return get();
    }

    constexpr value_type&& operator*() &&
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result_t' object" };
        }
        return std::move(*this).get();
    }

    constexpr const value_type& value() const& { return **this; }

    constexpr value_type& value() & { return **this; }

    constexpr value_type&& value() && { return *std::move(*this); }

    constexpr const value_type* operator->() const& { return &**this; }

    constexpr value_type* operator->() & { return &**this; }

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept { return static_cast<bool>(*this); }

    constexpr bool has_error() const noexcept { return !has_value(); }

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class T, class E>
struct result_t<T&, E>
{
    using value_type = T;
    using error_type = E;
    using value_storage = T*;
    using error_storage = error_wrapper_t<error_type>;

    constexpr result_t(value_type& value) : m_storage(std::in_place_type<value_storage>, &value) { }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(const error_wrapper_t<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(error_wrapper_t<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result_t(const result_t&) = default;
    constexpr result_t(result_t&&) noexcept = default;

    constexpr result_t& operator=(result_t other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const { return std::holds_alternative<value_storage>(m_storage); }

    constexpr value_type& get() const { return *std::get<value_storage>(m_storage); }

    constexpr value_type& operator*() const
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result_t' object" };
        }
        return get();
    }

    constexpr value_type* operator->() const { return &**this; }

    constexpr value_type& value() const { return **this; }

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept { return static_cast<bool>(*this); }

    constexpr bool has_error() const noexcept { return !has_value(); }

private:
    std::variant<value_storage, error_storage> m_storage;
};

template <class E>
struct result_t<void, E>
{
    using value_type = void;
    using error_type = E;

    constexpr result_t() : m_storage{} { }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(const error_wrapper_t<Err>& error) : m_storage(error.m_error)
    {
    }

    template <class Err, enable_if_t<std::is_constructible_v<error_type, Err>> = 0>
    constexpr result_t(error_wrapper_t<Err>&& error) : m_storage(std::move(error.m_error))
    {
    }

    constexpr result_t(const result_t&) = default;
    constexpr result_t(result_t&&) noexcept = default;

    constexpr result_t& operator=(result_t other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const { return !m_storage.has_value(); }

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return *m_storage;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return *m_storage;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result_t' object with value" };
        }
        return *std::move(m_storage);
    }

    constexpr bool has_value() const noexcept { return static_cast<bool>(*this); }

    constexpr bool has_error() const noexcept { return !has_value(); }

private:
    std::optional<error_type> m_storage;
};

template <
    class L,
    class LE,
    class R,
    class RE,
    class = std::invoke_result_t<std::equal_to<>, L, R>,
    class = std::invoke_result_t<std::equal_to<>, LE, RE>>
constexpr bool operator==(const result_t<L, LE>& lhs, const result_t<R, RE>& rhs)
{
    if (lhs.has_value() && rhs.has_value())
    {
        return *lhs == *rhs;
    }
    else if (lhs.has_error() && rhs.has_error())
    {
        return lhs.error() == rhs.error();
    }

    return false;
}

template <
    class L,
    class LE,
    class R,
    class RE,
    class = std::invoke_result_t<std::equal_to<>, L, R>,
    class = std::invoke_result_t<std::equal_to<>, LE, RE>>
constexpr bool operator!=(const result_t<L, LE>& lhs, const result_t<R, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class LE, class RE, class = std::invoke_result_t<std::equal_to<>, LE, RE>>
constexpr bool operator==(const result_t<void, LE>& lhs, const result_t<void, RE>& rhs)
{
    if (lhs.has_value() && rhs.has_value())
    {
        return true;
    }
    else if (lhs.has_error() && rhs.has_error())
    {
        return lhs.error() == rhs.error();
    }

    return false;
}

template <class LE, class RE, class = std::invoke_result_t<std::equal_to<>, LE, RE>>
constexpr bool operator!=(const result_t<void, LE>& lhs, const result_t<void, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class LE, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const result_t<L, LE>& lhs, const R& rhs)
{
    return lhs && *lhs == rhs;
}

template <class L, class LE, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const result_t<L, LE>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class RE, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const L& lhs, const result_t<R, RE>& rhs)
{
    return rhs && lhs == *rhs;
}

template <class L, class R, class RE, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const L& lhs, const result_t<R, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class T, class E, enable_if_t<has_ostream_operator<T>::value, has_ostream_operator<E>::value> = 0>
std::ostream& operator<<(std::ostream& os, const result_t<T, E>& result)
{
    if (result.has_value())
    {
        os << "result_t{ :value " << *result << " }";
    }
    else
    {
        os << "result_t{ :error " << result.error() << " }";
    }
    return os;
}

template <class E, enable_if_t<has_ostream_operator<E>::value> = 0>
std::ostream& operator<<(std::ostream& os, const result_t<void, E>& result)
{
    if (result.has_value())
    {
        os << "result_t{ :value void }";
    }
    else
    {
        os << "result_t{ :error " << result.error() << " }";
    }
    return os;
}

namespace result
{

namespace detail
{

struct try_invoke_fn
{
    template <class Func, class... Args>
    auto operator()(Func&& func, Args&&... args) const noexcept
        -> result_t<std::invoke_result_t<Func, Args...>, std::exception_ptr>
    {
        try
        {
            if constexpr (!std::is_void_v<std::invoke_result_t<Func, Args...>>)
            {
                return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            }
            else
            {
                std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
                return {};
            }
        }
        catch (...)
        {
            return error(std::current_exception());
        }
    }
};

template <class T>
struct try_create_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const noexcept -> result_t<T, std::exception_ptr>
    {
        try
        {
            return T{ std::forward<Args>(args)... };
        }
        catch (...)
        {
            return error(std::current_exception());
        }
    }
};

struct forward_error_fn
{
    template <class T, class E>
    auto operator()(result_t<T, E>&& result) const
    {
        if (result)
        {
            throw std::runtime_error("forward_error called with a result that has a value");
        }
        return error(std::move(result).error());
    }

    template <class T, class E>
    auto operator()(const result_t<T, E>& result) const
    {
        if (result)
        {
            throw std::runtime_error("forward_error called with a result that has a value");
        }
        return error(result.error());
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
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const T&>>,
            class Result = FuncResult>
        constexpr auto operator()(const result_t<T, E>& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func, value.get()) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = FuncResult>
        constexpr auto operator()(result_t<T, E>&& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func, std::move(value).get())
                         : ::zx::detail::to_error<Result>(std::move(value));
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&>>,
            class Result = FuncResult>
        constexpr auto operator()(const result_t<T&, E>& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func, value.get()) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = FuncResult>
        constexpr auto operator()(result_t<T&, E>&& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func, std::move(value).get())
                         : ::zx::detail::to_error<Result>(std::move(value));
        }

        template <
            class E,
            class F = Func,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const F&>>,
            class Result = FuncResult>
        constexpr auto operator()(const result_t<void, E>& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class E,
            class F = Func,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const F&>>,
            class Result = FuncResult>
        constexpr auto operator()(result_t<void, E>&& value) const -> Result
        {
            static_assert(
                ::zx::detail::is_result<FuncResult>::value,
                "and_then: function result type needs to be of `result_t<T, E>` type");
            return value ? ::zx::detail::to_ok<Result>(m_func) : ::zx::detail::to_error<Result>(std::move(value));
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ std::forward<Func>(func) });
    }
};

struct transform_fn
{
    template <class Func>
    struct impl_t
    {
        Func m_func;

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const T&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(const result_t<T, E>& value) const -> Result
        {
            return value ? ::zx::detail::to_ok<Result>(m_func, value.get()) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(result_t<T, E>&& value) const -> Result
        {
            return value  //
                       ? ::zx::detail::to_ok<Result>(m_func, std::move(value).get())
                       : ::zx::detail::to_error<Result>(std::move(value));
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(const result_t<T&, E>& value) const -> Result
        {
            return value ? ::zx::detail::to_ok<Result>(m_func, value.get()) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, T&&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(result_t<T&, E>&& value) const -> Result
        {
            return value  //
                       ? ::zx::detail::to_ok<Result>(m_func, std::move(value).get())
                       : ::zx::detail::to_error<Result>(std::move(value));
        }

        template <
            class E,
            class F = Func,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const F&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(const result_t<void, E>& value) const -> Result
        {
            return value ? ::zx::detail::to_ok<Result>(m_func) : ::zx::detail::to_error<Result>(value);
        }

        template <
            class E,
            class F = Func,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const F&>>,
            class Result = result_t<FuncResult, E>>
        constexpr auto operator()(result_t<void, E>&& value) const -> Result
        {
            return value ? ::zx::detail::to_ok<Result>(m_func) : ::zx::detail::to_error<Result>(std::move(value));
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ std::forward<Func>(func) });
    }
};

struct transform_error_fn
{
    template <class Func>
    struct impl_t
    {
        Func m_func;

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<T, FuncResult>>
        constexpr auto operator()(const result_t<T, E>& value) const -> Result
        {
            return value ? Result{ value.get() } : Result{ zx::error(std::invoke(m_func, value.error())) };
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<T, FuncResult>>
        constexpr auto operator()(result_t<T, E>&& value) const -> Result
        {
            return value ? Result{ std::move(value).get() }
                         : Result{ zx::error(std::invoke(m_func, std::move(value).error())) };
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<T, FuncResult>>
        constexpr auto operator()(const result_t<T&, E>& value) const -> Result
        {
            return value ? Result{ value.get() } : Result{ zx::error(std::invoke(m_func, value.error())) };
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<T, FuncResult>>
        constexpr auto operator()(result_t<T&, E>&& value) const -> Result
        {
            return value ? Result{ std::move(value).get() }
                         : Result{ zx::error(std::invoke(m_func, std::move(value).error())) };
        }

        template <
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<void, FuncResult>>
        constexpr auto operator()(const result_t<void, E>& value) const -> Result
        {
            return value ? Result{} : Result{ zx::error(std::invoke(m_func, value.error())) };
        }

        template <
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<Func, const E&>>,
            class Result = result_t<void, FuncResult>>
        constexpr auto operator()(result_t<void, E>&& value) const -> Result
        {
            return value  //
                       ? Result{}
                       : Result{ zx::error(std::invoke(m_func, std::move(value).error())) };
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ func });
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
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<T, E>, FuncResult>>
        constexpr auto operator()(const result_t<T, E>& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value ? Result{ value } : (std::invoke(m_func, value.error()), Result{ value });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value ? Result{ value } : Result{ std::invoke(m_func, value.error()) };
            }
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<T, E>, FuncResult>>
        constexpr auto operator()(result_t<T, E>&& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value ? Result{ std::move(value) }
                             : (std::invoke(m_func, std::move(value).error()), Result{ std::move(value) });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value ? Result{ std::move(value) } : Result{ std::invoke(m_func, std::move(value).error()) };
            }
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<T, E>, FuncResult>>
        constexpr auto operator()(const result_t<T&, E>& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value ? Result{ value } : (std::invoke(m_func, value.error()), Result{ value });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value ? Result{ value } : Result{ std::invoke(m_func, value.error()) };
            }
        }

        template <
            class T,
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<T, E>, FuncResult>>
        constexpr auto operator()(result_t<T&, E>&& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value ? Result{ std::move(value) } : (std::invoke(m_func, value.error()), Result{ std::move(value) });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value ? Result{ std::move(value) } : Result{ std::invoke(m_func, std::move(value).error()) };
            }
        }

        template <
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<void, E>, FuncResult>>
        constexpr auto operator()(const result_t<void, E>& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value  //
                           ? Result{ value }
                           : (std::invoke(m_func, value.error()), Result{ value });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value  //
                           ? Result{ value }
                           : Result{ std::invoke(m_func, value.error()) };
            }
        }

        template <
            class E,
            class FuncResult = remove_rvalue_reference_t<std::invoke_result_t<const Func&, const E&>>,
            class Result = std::conditional_t<std::is_void_v<FuncResult>, result_t<void, E>, FuncResult>>
        constexpr auto operator()(result_t<void, E>&& value) const -> Result
        {
            if constexpr (std::is_void_v<FuncResult>)
            {
                return value ? Result{ std::move(value) } : (std::invoke(m_func, value.error()), Result{ std::move(value) });
            }
            else
            {
                static_assert(
                    ::zx::detail::is_result<FuncResult>::value,
                    "or_else: function result type needs to be of `result_t<T, E>` type");
                return value ? Result{ std::move(value) } : Result{ std::invoke(m_func, std::move(value).error()) };
            }
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const
    {
        return fn(impl_t<std::decay_t<Func>>{ func });
    }
};

struct value_or_fn
{
    template <class U>
    struct impl_t
    {
        U m_default_value;

        template <class T, class E>
        constexpr auto operator()(const result_t<T, E>& value) const& -> T
        {
            return value.has_value() ? *value : static_cast<T>(m_default_value);
        }

        template <class T, class E>
        constexpr auto operator()(result_t<T, E>&& value) && -> T
        {
            return value.has_value() ? *value : static_cast<T>(m_default_value);
        }

        template <class T, class E>
        constexpr auto operator()(const result_t<T&, E>& value) const -> T
        {
            return value.has_value() ? *value : static_cast<T>(m_default_value);
        }

        template <class T, class E>
        constexpr auto operator()(result_t<T&, E>&& value) && -> T
        {
            return value.has_value() ? *value : static_cast<T>(m_default_value);
        }
    };

    template <class U>
    constexpr auto operator()(U&& default_value) const
    {
        return fn(impl_t<std::decay_t<U>>{ default_value });
    }
};

}  // namespace detail

constexpr inline auto and_then = detail::and_then_fn{};
constexpr inline auto transform = detail::transform_fn{};
constexpr inline auto transform_error = detail::transform_error_fn{};
constexpr inline auto or_else = detail::or_else_fn{};
constexpr inline auto value_or = detail::value_or_fn{};

template <class T>
constexpr inline auto try_create = detail::try_create_fn<T>{};

constexpr inline auto try_invoke = detail::try_invoke_fn{};

constexpr inline auto forward_error = detail::forward_error_fn{};

}  // namespace result

}  // namespace zx
