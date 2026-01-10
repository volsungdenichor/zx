#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace zx
{

template <class T, class E>
struct result;

namespace detail
{

template <class T>
struct is_result : std::false_type
{
};

template <class T, class E>
struct is_result<result<T, E>> : std::true_type
{
};

template <class E>
struct error_wrapper
{
    E m_error;
};

template <class E>
constexpr auto error(E&& error) -> error_wrapper<std::decay_t<E>>
{
    return error_wrapper<std::decay_t<E>>{ std::forward<E>(error) };
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
struct to_result<result<void, E>>
{
    template <class Func, class... Args>
    constexpr auto operator()(Func&& func, Args&&... args) const -> result<void, E>
    {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        return result<void, E>{};
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
using detail::error_wrapper;

struct bad_result_access : std::runtime_error
{
    explicit bad_result_access(const std::string& msg) : std::runtime_error{ msg }
    {
    }
};

template <class T, class E>
struct result
{
    using value_type = T;
    using error_type = E;
    using value_storage = value_type;
    using error_storage = error_wrapper<error_type>;

    constexpr result()
    {
    }

    constexpr result(value_type value) : m_storage(std::in_place_type<value_storage>, std::move(value))
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr result& operator=(result other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr const value_type& get() const&
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& get() &
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& get() &&
    {
        return std::get<value_storage>(std::move(m_storage));
    }

    constexpr const value_type& operator*() const&
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result' object" };
        }
        return get();
    }

    constexpr value_type& operator*() &
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result' object" };
        }
        return get();
    }

    constexpr value_type&& operator*() &&
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result' object" };
        }
        return std::move(*this).get();
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

    constexpr const value_type* operator->() const&
    {
        return &**this;
    }

    constexpr value_type* operator->() &
    {
        return &**this;
    }

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), get())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).get())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), get())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).get())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ get() }
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).get() }
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
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
    std::variant<value_storage, error_storage> m_storage;
};

template <class T, class E>
struct result<T&, E>
{
    using value_type = T;
    using error_type = E;
    using value_storage = std::reference_wrapper<value_type>;
    using error_storage = error_wrapper<error_type>;

    constexpr result(value_type& value) : m_storage(std::in_place_type<value_storage>, value)
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ error.m_error })
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error)
        : m_storage(std::in_place_type<error_storage>, error_storage{ std::move(error.m_error) })
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr result& operator=(result other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr value_type& get() const
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& operator*() const
    {
        if (!has_value())
        {
            throw bad_result_access{ "accessing the value of an empty 'result' object" };
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

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return std::get<error_storage>(std::move(m_storage)).m_error;
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), get())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).get())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), get())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), get())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<T, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ get() }
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).get() }
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
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
    std::variant<value_storage, error_storage> m_storage;
};

template <class E>
struct result<void, E>
{
    using value_type = void;
    using error_type = E;

    constexpr result() : m_storage{}
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(const error_wrapper<Err>& error) : m_storage(error.m_error)
    {
    }

    template <class Err, class = std::enable_if_t<std::is_constructible_v<error_type>>>
    constexpr result(error_wrapper<Err>&& error) : m_storage(std::move(error.m_error))
    {
    }

    constexpr result(const result&) = default;
    constexpr result(result&&) noexcept = default;

    constexpr result& operator=(result other)
    {
        std::swap(m_storage, other.m_storage);
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return !m_storage.has_value();
    }

    constexpr const error_type& error() const&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return *m_storage;
    }

    constexpr error_type& error() &
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return *m_storage;
    }

    constexpr error_type&& error() &&
    {
        if (!has_error())
        {
            throw bad_result_access{ "accessing the error of a 'result' object with value" };
        }
        return *std::move(m_storage);
    }

    constexpr bool has_value() const noexcept
    {
        return static_cast<bool>(*this);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) const& -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func))
                   : detail::to_error<Result>(std::move(*this));
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<void, E>, FuncResult>>
    constexpr auto or_else(Func&& func) const& -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ *this }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ *this });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ *this }
                       : Result{ std::invoke(std::forward<Func>(func), error()) };
        }
    }

    template <
        class Func,
        class FuncResult = std::invoke_result_t<Func, const E&>,
        class Result = std::conditional_t<std::is_void_v<FuncResult>, result<void, E>, FuncResult>>
    constexpr auto or_else(Func&& func) && -> Result
    {
        if constexpr (std::is_void_v<FuncResult>)
        {
            return *this  //
                       ? Result{ std::move(*this) }
                       : (std::invoke(std::forward<Func>(func), error()), Result{ std::move(*this) });
        }
        else
        {
            static_assert(
                detail::is_result<FuncResult>::value, "or_else: function result type needs to be of `result<T, E>` type");
            return *this  //
                       ? Result{ std::move(*this) }
                       : Result{ std::invoke(std::forward<Func>(func), std::move(*this).error()) };
        }
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<void, FuncResult>>
    constexpr auto transform_error(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{}
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<void, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{}
                   : Result{ zx::error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
    }

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
constexpr bool operator==(const result<L, LE>& lhs, const result<R, RE>& rhs)
{
    if (lhs.has_value() && rhs.has_value())
    {
        return lhs.get() == rhs.get();
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
constexpr bool operator!=(const result<L, LE>& lhs, const result<R, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class LE, class RE, class = std::invoke_result_t<std::equal_to<>, LE, RE>>
constexpr bool operator==(const result<void, LE>& lhs, const result<void, RE>& rhs)
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
constexpr bool operator!=(const result<void, LE>& lhs, const result<void, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class L, class LE, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const result<L, LE>& lhs, const R& rhs)
{
    return lhs && lhs.get() == rhs;
}

template <class L, class LE, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const result<L, LE>& lhs, const R& rhs)
{
    return !(lhs == rhs);
}

template <class L, class R, class RE, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const L& lhs, const result<R, RE>& rhs)
{
    return rhs && lhs == rhs.get();
}

template <class L, class R, class RE, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const L& lhs, const result<R, RE>& rhs)
{
    return !(lhs == rhs);
}

template <class Func, class... Args>
auto try_invoke(Func&& func, Args&&... args) noexcept -> result<std::invoke_result_t<Func, Args...>, std::exception_ptr>
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

template <class T, class... Args>
auto try_create(Args&&... args) noexcept -> result<T, std::exception_ptr>
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

}  // namespace zx
