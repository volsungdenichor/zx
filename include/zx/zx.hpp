#pragma once

#include <algorithm>
#include <cstdint>
#include <cuchar>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>
#ifdef __GNUG__
#include <cxxabi.h>

#include <cstdlib>
#include <memory>

#endif  // __GNUG__

namespace zx
{

using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using i16 = std::int16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;

namespace detail
{

template <class AlwaysVoid, template <class...> class Op, class... Args>
struct detector_impl : std::false_type
{
};

template <template <class...> class Op, class... Args>
struct detector_impl<std::void_t<Op<Args...>>, Op, Args...> : std::true_type
{
};

template <typename T>
constexpr bool should_pass_by_value = sizeof(T) <= 2 * sizeof(void*) && std::is_trivially_copy_constructible_v<T>;

}  // namespace detail

template <class T>
using in_t = std::conditional_t<detail::should_pass_by_value<T>, const T, const T&>;

template <class T>
using return_t = std::conditional_t<detail::should_pass_by_value<T>, T, const T&>;

template <bool C>
using require = std::enable_if_t<C, int>;

template <class T>
struct type_identity
{
    using type = T;
};

template <class T>
using type_identity_t = typename type_identity<T>::type;

template <class...>
struct always_false : std::false_type
{
};

template <template <class...> class Op, class... Args>
struct is_detected : detail::detector_impl<std::void_t<>, Op, Args...>
{
};

template <class T>
using iterator_t = decltype(std::begin(std::declval<T&>()));

template <class T>
using iter_category_t = typename std::iterator_traits<T>::iterator_category;

template <class T>
using iter_reference_t = typename std::iterator_traits<T>::reference;

template <class T>
using iter_value_t = typename std::iterator_traits<T>::value_type;

template <class T>
using iter_difference_t = typename std::iterator_traits<T>::difference_type;

template <class T>
using range_category_t = iter_category_t<iterator_t<T>>;

template <class T>
using range_reference_t = iter_reference_t<iterator_t<T>>;

template <class T>
using range_value_t = iter_value_t<iterator_t<T>>;

template <class T>
using range_difference_t = iter_difference_t<iterator_t<T>>;

namespace detail
{
template <class Category, class T>
struct iterator_of_category : std::is_base_of<Category, typename std::iterator_traits<T>::iterator_category>
{
};
}  // namespace detail

template <class T>
struct is_input_iterator : detail::iterator_of_category<std::input_iterator_tag, T>
{
};

template <class T>
struct is_forward_iterator : detail::iterator_of_category<std::forward_iterator_tag, T>
{
};

template <class T>
struct is_bidirectional_iterator : detail::iterator_of_category<std::bidirectional_iterator_tag, T>
{
};

template <class T>
struct is_random_access_iterator : detail::iterator_of_category<std::random_access_iterator_tag, T>
{
};

template <class T>
struct is_input_range : is_input_iterator<iterator_t<T>>
{
};

template <class T>
struct is_forward_range : is_forward_iterator<iterator_t<T>>
{
};

template <class T>
struct is_bidirectional_range : is_bidirectional_iterator<iterator_t<T>>
{
};

template <class T>
struct is_random_access_range : is_random_access_iterator<iterator_t<T>>
{
};

template <class Os, class T>
using has_ostream_operator_impl = decltype(std::declval<Os>() << std::declval<T>());

template <class T, class Os = std::ostream>
struct has_ostream_operator : is_detected<has_ostream_operator_impl, Os, T>
{
};

#ifdef __GNUG__

inline std::string demangle(const char* name)
{
    int status = -4;
    std::unique_ptr<char, void (*)(void*)> res{ abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
    return (status == 0) ? res.get() : name;
}

#else

// does nothing if not g++
inline std::string demangle(const char* name)
{
    return name;
}

#endif  // __GNUG__

template <class T, class = void>
struct formatter;

namespace detail
{

static constexpr inline struct format_to_fn
{
    template <class T>
    using has_formatter_impl = decltype(formatter<T>{});

    template <class T>
    static void do_format(std::ostream& os, const T& item)
    {
        if constexpr (is_detected<has_formatter_impl, T>::value)
        {
            formatter<T>{}.format(os, item);
        }
        else if constexpr (has_ostream_operator<T>::value)
        {
            os << item;
        }
        else
        {
            os << "[" << demangle(typeid(T).name()) << "]";
        }
    }

    template <class... Args>
    std::ostream& operator()(std::ostream& os, const Args&... args) const
    {
        (do_format(os, args), ...);
        return os;
    }
} format_to;

static constexpr inline struct format_fn
{
    template <class... Args>
    auto operator()(const Args&... args) const -> std::string
    {
        std::stringstream ss;
        format_to(ss, args...);
        return std::move(ss).str();
    }
} format;

static constexpr inline struct print_fn
{
    template <class... Args>
    std::ostream& operator()(const Args&... args) const
    {
        return format_to(std::cout, args...);
    }
} print;

static constexpr inline struct println_fn
{
    template <class... Args>
    std::ostream& operator()(const Args&... args) const
    {
        return print(args...) << std::endl;
    }
} println;

}  // namespace detail

using detail::format;
using detail::format_to;
using detail::print;
using detail::println;
static constexpr inline auto str = detail::format_fn{};

template <>
struct formatter<std::exception_ptr>
{
    void format(std::ostream& os, const std::exception_ptr& item) const
    {
        try
        {
            std::rethrow_exception(item);
        }
        catch (const std::exception& ex)
        {
#ifdef __GNUG__
            format_to(
                os, "std::exception_ptr<", demangle(abi::__cxa_current_exception_type()->name()), ">(\"", ex.what(), "\")");
#else
            os << "exception<"
               << "std::exception"
               << ">(" << ex.what() << ")";
#endif  // __GNUG__
        }

        catch (const std::string& ex)
        {
            format_to(os, "std::exception_ptr<std::string>(\"", ex, "\")");
        }
        catch (const char* ex)
        {
            format_to(os, "std::exception_ptr<const char*>(\"", ex, "\")");
        }
        catch (...)
        {
            format_to(os, "std::exception_ptr<...>(\"\")");
        }
    }
};

template <class T, class E>
struct result;

template <class T>
struct maybe;

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

/*

# result<T, E>

*/

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

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr const value_type& value() const&
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& value() &
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& value() &&
    {
        return std::get<value_storage>(std::move(m_storage));
    }

    constexpr const value_type& operator*() const&
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& operator*() &
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& operator*() &&
    {
        return std::get<value_storage>(std::move(m_storage));
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
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
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
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
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
                   ? Result{ value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
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

    constexpr explicit operator bool() const
    {
        return std::holds_alternative<value_storage>(m_storage);
    }

    constexpr value_type& operator*() const
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type* operator->() const
    {
        return &**this;
    }

    constexpr value_type& value() const
    {
        return std::get<value_storage>(m_storage);
    }

    constexpr const error_type& error() const&
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
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
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(
            detail::is_result<FuncResult>::value, "and_then: function result type needs to be of `result<T, E>` type");
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), std::move(*this).value())
                   : detail::to_error<Result>(std::move(*this));
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
                   : detail::to_error<Result>(*this);
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = result<FuncResult, E>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? detail::to_ok<Result>(std::forward<Func>(func), value())
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
                   ? Result{ value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<T, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::move(*this).value() }
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
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

    constexpr explicit operator bool() const
    {
        return !m_storage.has_value();
    }

    constexpr const error_type& error() const&
    {
        return *m_storage;
    }

    constexpr error_type& error() &
    {
        return *m_storage;
    }

    constexpr error_type&& error() &&
    {
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
                   : Result{ error(std::invoke(std::forward<Func>(func), error())) };
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const E&>, class Result = result<void, FuncResult>>
    constexpr auto transform_error(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{}
                   : Result{ error(std::invoke(std::forward<Func>(func), std::move(*this).error())) };
    }

private:
    std::optional<error_type> m_storage;
};

template <class E>
struct formatter<result<void, E>>
{
    void format(std::ostream& os, const result<void, E>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "ok()");
        }
        else
        {
            format_to(os, "error(", item.error(), ")");
        }
    }
};

template <class T, class E>
struct formatter<result<T, E>>
{
    void format(std::ostream& os, const result<T, E>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "ok(", item.value(), ")");
        }
        else
        {
            format_to(os, "error(", item.error(), ")");
        }
    }
};

template <class T, class E>
std::ostream& operator<<(std::ostream& os, const result<T, E>& item)
{
    return format_to(os, item);
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

template <class T>
struct maybe_underlying_type;

template <class T>
using maybe_underlying_type_t = typename maybe_underlying_type<T>::type;

template <class T>
struct maybe_underlying_type<maybe<T>>
{
    using type = T;
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

/*

# maybe

*/

/*

## maybe<T>

*/

template <class T>
struct maybe
{
    using value_type = T;

    constexpr maybe() : m_storage()
    {
    }

    constexpr maybe(value_type value) : m_storage(std::move(value))
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
        return m_storage.has_value();
    }

    constexpr const value_type& operator*() const&
    {
        return *m_storage;
    }

    constexpr value_type& operator*() &
    {
        return *m_storage;
    }

    constexpr value_type&& operator*() &&
    {
        return *std::move(m_storage);
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
        return *m_storage;
    }

    constexpr value_type& value() &
    {
        return *m_storage;
    }

    constexpr value_type&& value() &&
    {
        return *std::move(m_storage);
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
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = FuncResult>
    constexpr auto and_then(Func&& func) && -> Result
    {
        static_assert(detail::is_maybe<FuncResult>::value, "and_then: function result type needs to be of `maybe<T>` type");
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, const T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const& -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) && -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), std::move(*this).value()) }
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
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ *this }
                   : maybe<T>{};
    }

    template <class Pred>
    constexpr auto filter(Pred&& pred) && -> maybe<T>
    {
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ std::move(*this) }
                   : maybe<T>{};
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

    constexpr value_type& operator*() const
    {
        return *m_storage;
    }

    constexpr value_type* operator->() const
    {
        return &**this;
    }

    constexpr value_type& value() const
    {
        return *m_storage;
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
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
                   : Result{};
    }

    template <class Func, class FuncResult = std::invoke_result_t<Func, T&>, class Result = maybe<FuncResult>>
    constexpr auto transform(Func&& func) const -> Result
    {
        return *this  //
                   ? Result{ std::invoke(std::forward<Func>(func), value()) }
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
        return std::invoke(std::forward<Pred>(pred), value())  //
                   ? maybe<T>{ *this }
                   : maybe<T>{};
    }

private:
    value_type* m_storage;
};

template <class T>
struct formatter<maybe<T>>
{
    void format(std::ostream& os, const maybe<T>& item) const
    {
        if (item.has_value())
        {
            format_to(os, "some(", item.value(), ")");
        }
        else
        {
            format_to(os, "none");
        }
    }
};

template <class T>
std::ostream& operator<<(std::ostream& os, const maybe<T>& item)
{
    return format_to(os, item);
}

namespace detail
{

template <class Iter>
struct reverse_iterator_impl
{
    using type = std::reverse_iterator<Iter>;
};

template <class Iter>
struct reverse_iterator_impl<std::reverse_iterator<Iter>>
{
    using type = Iter;
};

template <class Iter>
using reverse_iterator_t = typename reverse_iterator_impl<Iter>::type;

template <class Iter>
auto make_reverse(Iter b, Iter e) -> std::pair<reverse_iterator_t<Iter>, reverse_iterator_t<Iter>>
{
    return { reverse_iterator_t<Iter>(e), reverse_iterator_t<Iter>(b) };
}

template <class Iter>
auto make_reverse(std::reverse_iterator<Iter> b, std::reverse_iterator<Iter> e) -> std::pair<Iter, Iter>
{
    return { e.base(), b.base() };
}

}  // namespace detail

/*

# iterator_range<Iter>

*/

struct slice_t
{
    maybe<std::ptrdiff_t> begin;
    maybe<std::ptrdiff_t> end;
};

template <class Iter>
class iterator_range
{
public:
    using iterator = Iter;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = difference_type;

    using maybe_reference = maybe<reference>;

    using reverse_type = iterator_range<detail::reverse_iterator_t<iterator>>;

    constexpr iterator_range() = default;
    constexpr iterator_range(const iterator_range&) = default;
    constexpr iterator_range(iterator_range&&) noexcept = default;

    constexpr iterator_range(iterator b, iterator e) : m_begin(b), m_end(e)
    {
    }

    constexpr iterator_range(const std::pair<iterator, iterator>& pair)
        : iterator_range(std::get<0>(pair), std::get<1>(pair))
    {
    }

    constexpr iterator_range(iterator b, size_type n) : iterator_range(b, std::next(b, n))
    {
    }

    template <class Range, class = iterator_t<Range>>
    constexpr iterator_range(Range&& range) : iterator_range(std::begin(range), std::end(range))
    {
    }

    iterator_range& operator=(iterator_range other) noexcept
    {
        std::swap(m_begin, other.m_begin);
        std::swap(m_end, other.m_end);
        return *this;
    }

    constexpr auto begin() const noexcept -> iterator
    {
        return m_begin;
    }

    constexpr auto end() const noexcept -> iterator
    {
        return m_end;
    }

    template <class Container, require<std::is_constructible_v<Container, iterator, iterator>> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto empty() const -> bool
    {
        return begin() == end();
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto ssize() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto size() const -> difference_type
    {
        return std::distance(begin(), end());
    }

    auto front() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::front - empty range" };
        }
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto back() const -> reference
    {
        if (empty())
        {
            throw std::out_of_range{ "iterator_range::back - empty range" };
        }
        return *std::prev(end());
    }

    auto maybe_front() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto maybe_back() const -> maybe_reference
    {
        if (empty())
        {
            return {};
        }
        return *std::prev(end());
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto at(difference_type n) const -> reference
    {
        if (!(0 <= n && n < size()))
        {
            throw std::out_of_range{ "iterator_range::at - index out of range" };
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto operator[](difference_type n) const -> reference
    {
        return at(n);
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto maybe_at(difference_type n) const -> maybe_reference
    {
        if (!(0 <= n && n < size()))
        {
            return {};
        }
        return *std::next(begin(), n);
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto reverse() const -> reverse_type
    {
        return detail::make_reverse(begin(), end());
    }

    auto take(difference_type n) const -> iterator_range
    {
        return iterator_range(begin(), advance(n));
    }

    auto drop(difference_type n) const -> iterator_range
    {
        return iterator_range(advance(n), end());
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto take_back(difference_type n) const -> iterator_range
    {
        return reverse().take(n).reverse();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto drop_back(difference_type n) const -> iterator_range
    {
        return reverse().drop(n).reverse();
    }

    template <class Pred>
    auto take_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(begin(), found);
    }

    template <class Pred>
    auto drop_while(Pred&& pred) const -> iterator_range
    {
        const iterator found = std::find_if_not(begin(), end(), std::forward<Pred>(pred));
        return iterator_range(found, end());
    }

    template <class Pred, class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto take_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().take_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred, class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto drop_back_while(Pred&& pred) const -> iterator_range
    {
        return reverse().drop_while(std::forward<Pred>(pred)).reverse();
    }

    template <class Pred>
    auto find_if(Pred&& pred) const -> maybe_reference
    {
        const iterator found = std::find_if(begin(), end(), std::forward<Pred>(pred));
        return found != end() ? maybe_reference{ *found } : maybe_reference{};
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto slice(const slice_t& info) const -> iterator_range
    {
        static const auto adjust = [](difference_type index, size_type size) -> size_type {  //
            return std::clamp<size_type>(index >= 0 ? index : index + size, 0, size);
        };
        const size_type s = size();
        const size_type b = info.begin ? adjust(*info.begin, s) : size_type{ 0 };
        const size_type e = info.end ? adjust(*info.end, s) : s;
        return iterator_range{ begin() + b, std::max(size_type{ 0 }, e - b) };
    }

private:
    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        return begin() + std::min(ssize(), n);
    }

    template <class It = iterator, require<!is_random_access_iterator<It>::value> = 0>
    auto advance(difference_type n) const -> iterator
    {
        iterator it = begin();
        while (it != end() && n > 0)
        {
            --n;
            ++it;
        }
        return it;
    }

    iterator m_begin;
    iterator m_end;
};

template <class Container>
using subrange = iterator_range<iterator_t<Container>>;

template <class T>
using span = iterator_range<const T*>;

template <class T>
using mut_span = iterator_range<T*>;

struct identity
{
    template <class T>
    constexpr T&& operator()(T&& item) const noexcept
    {
        return std::forward<T>(item);
    }
};

template <class... Args>
struct formatter<std::tuple<Args...>>
{
    void format(std::ostream& os, const std::tuple<Args...>& item) const
    {
        format_to(os, "(");
        std::apply(
            [&os](const auto&... args)
            {
                auto n = 0u;
                ((format_to(os, args) << (++n != sizeof...(args) ? ", " : "")), ...);
            },
            item);
        format_to(os, ")");
    }
};

template <class F, class S>
struct formatter<std::pair<F, S>>
{
    void format(std::ostream& os, const std::pair<F, S>& item) const
    {
        format_to(os, "(", item.first, ", ", item.second, ")");
    }
};

/*

# pipe_t<Pipes...>

*/

template <class... Pipes>
struct pipe_t
{
    std::tuple<Pipes...> m_pipes;

    constexpr pipe_t(std::tuple<Pipes...> pipes) : m_pipes{ std::move(pipes) }
    {
    }

    constexpr pipe_t(Pipes... pipes) : pipe_t{ std::tuple<Pipes...>{ std::move(pipes)... } }
    {
    }

private:
    template <std::size_t I, class... Args>
    constexpr auto invoke(Args&&... args) const -> decltype(std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...))
    {
        return std::invoke(std::get<I>(m_pipes), std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) == sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(invoke<I>(std::forward<Args>(args)...))
    {
        return invoke<I>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, std::enable_if_t<(I + 1) < sizeof...(Pipes), int> = 0>
    constexpr auto call(Args&&... args) const -> decltype(call<I + 1>(invoke<I>(std::forward<Args>(args)...)))
    {
        return call<I + 1>(invoke<I>(std::forward<Args>(args)...));
    }

public:
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(call<0>(std::forward<Args>(args)...))
    {
        return call<0>(std::forward<Args>(args)...);
    }
};

template <class T>
struct is_pipeline : std::false_type
{
};

template <class... Args>
struct is_pipeline<pipe_t<Args...>> : std::true_type
{
};

template <class... L, class... R>
constexpr auto operator|=(pipe_t<L...> lhs, pipe_t<R...> rhs) -> decltype(pipe(std::move(lhs), std::move(rhs)))
{
    return pipe(std::move(lhs), std::move(rhs));
}

template <class T, class... Pipes, std::enable_if_t<!is_pipeline<std::decay_t<T>>{}, int> = 0>
constexpr auto operator|=(T&& item, const pipe_t<Pipes...>& p) -> decltype(p(std::forward<T>(item)))
{
    return p(std::forward<T>(item));
}

template <class... Pipes>
struct formatter<pipe_t<Pipes...>>
{
    void format(std::ostream& os, const pipe_t<Pipes...>& item) const
    {
        format_to(os, "pipe(");
        std::apply(
            [&os](const auto&... args)
            {
                auto n = 0u;
                ((format_to(os, args) << (++n != sizeof...(args) ? ", " : "")), ...);
            },
            item.m_pipes);
        format_to(os, ")");
    }
};

namespace detail
{

struct pipe_fn
{
private:
    template <class Pipe>
    constexpr auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    constexpr auto to_tuple(pipe_t<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_pipes;
    }

    template <class... Pipes>
    constexpr auto from_tuple(std::tuple<Pipes...> tuple) const -> pipe_t<Pipes...>
    {
        return pipe_t<Pipes...>{ std::move(tuple) };
    }

public:
    template <class... Pipes>
    constexpr auto operator()(Pipes&&... pipes) const
        -> decltype(from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::forward<Pipes>(pipes))...));
    }
};

static constexpr inline struct do_all_fn
{
    template <class... Funcs>
    struct impl
    {
        std::tuple<Funcs...> m_funcs;

        template <class... Args>
        void operator()(Args&&... args) const
        {
            call(std::index_sequence_for<Funcs...>{}, std::forward<Args>(args)...);
        }

        template <class... Args, std::size_t... I>
        void call(std::index_sequence<I...>, Args&&... args) const
        {
            (std::invoke(std::get<I>(m_funcs), std::forward<Args>(args)...), ...);
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> impl<std::decay_t<Funcs>...>
    {
        return { std::forward_as_tuple(std::forward<Funcs>(funcs)...) };
    }
} do_all;

static constexpr struct apply_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T& operator()(T& item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> pipe_t<impl<decltype(do_all(std::forward<Funcs>(funcs)...))>>
    {
        return { { do_all(std::forward<Funcs>(funcs)...) } };
    }
} apply;

static constexpr inline struct with_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class T>
        T operator()(T item) const
        {
            std::invoke(m_func, item);
            return item;
        }
    };

    template <class... Funcs>
    auto operator()(Funcs&&... funcs) const -> pipe_t<impl<decltype(do_all(std::forward<Funcs>(funcs)...))>>
    {
        return { { do_all(std::forward<Funcs>(funcs)...) } };
    }
} with;

}  // namespace detail

static constexpr inline auto pipe = detail::pipe_fn{};
using detail::apply;
using detail::do_all;
using detail::with;

/*

# sequence<T>

*/

template <class T>
using iteration_result_t = maybe<T>;

template <class T>
using next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
struct sequence;

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
    auto inspect(Func&& func) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect(Func&& func) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto inspect_indexed(Func&& func) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func>
    auto inspect_indexed(Func&& func) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Func>>{ std::forward<Func>(func),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto transform(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto transform(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto transform_indexed(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, std::ptrdiff_t, T>>
    auto transform_indexed(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto transform_maybe(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, T>>>
    auto transform_maybe(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
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
        mutable std::ptrdiff_t m_index;

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
    auto transform_maybe_indexed(Func&& func) const& -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = maybe_underlying_type_t<std::invoke_result_t<Func, std::ptrdiff_t, T>>>
    auto transform_maybe_indexed(Func&& func) && -> sequence<Res>
    {
        return sequence<Res>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto filter(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto filter_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto filter_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto drop_while(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto drop_while_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto drop_while_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto take_while(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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
    auto take_while_indexed(Pred&& pred) const& -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{
            std::forward<Pred>(pred), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Pred>
    auto take_while_indexed(Pred&& pred) && -> sequence<T>
    {
        return sequence<T>{ next_function<std::decay_t<Pred>>{ std::forward<Pred>(pred),
                                                               static_cast<sequence<T>&&>(*this).get_next_function() } };
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

    auto drop(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto drop(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
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

    auto take(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto take(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
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

    auto step(std::ptrdiff_t n) const& -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    auto step(std::ptrdiff_t n) && -> sequence<T>
    {
        return sequence<T>{ next_function{ n, static_cast<sequence<T>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct join_mixin
{
};

template <class T>
struct join_mixin<sequence<T>>
{
    struct next_function
    {
        next_function_t<sequence<T>> m_next;
        mutable next_function_t<T> m_sub = {};

        auto operator()() const -> iteration_result_t<T>
        {
            while (true)
            {
                if (!m_sub)
                {
                    iteration_result_t<sequence<T>> next = m_next();
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

    auto join() const& -> sequence<T>
    {
        return sequence<T>{ next_function{ static_cast<const sequence<sequence<T>>&>(*this).get_next_function() } };
    }

    auto join() && -> sequence<T>
    {
        return sequence<T>{ next_function{ static_cast<sequence<sequence<T>>&&>(*this).get_next_function() } };
    }
};

template <class T>
struct for_each_mixin
{
    template <class Func>
    void for_each(Func&& func) const&
    {
        const auto next_function = static_cast<const sequence<T>&>(*this).get_next_function();
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
        const auto next_function = static_cast<const sequence<T>&>(*this).get_next_function();
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
struct empty_sequence
{
    auto operator()() const -> iteration_result_t<T>
    {
        return {};
    }
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

template <class T>
struct pointer_proxy
{
    T item;

    T* operator->()
    {
        return std::addressof(item);
    }
};

template <class T>
struct sequence_iterator
{
    using next_function_type = next_function_t<T>;
    using reference = T;
    using difference_type = std::ptrdiff_t;
    using value_type = std::decay_t<reference>;
    using pointer
        = std::conditional_t<std::is_reference_v<reference>, std::add_pointer_t<reference>, pointer_proxy<reference>>;
    using iterator_category = std::forward_iterator_tag;

    next_function_type m_next_fn;
    iteration_result_t<reference> m_current;
    difference_type m_index;

    sequence_iterator() : m_next_fn{}, m_current{}, m_index{ std::numeric_limits<difference_type>::max() }
    {
    }

    sequence_iterator(const next_function_type& next_fn) : m_next_fn{ next_fn }, m_current{ m_next_fn() }, m_index{ 0 }
    {
    }

    sequence_iterator(const sequence_iterator&) = default;
    sequence_iterator(sequence_iterator&&) noexcept = default;

    sequence_iterator& operator=(sequence_iterator other)
    {
        std::swap(m_next_fn, m_next_fn);
        return *this;
    }

    reference operator*() const
    {
        return *m_current;
    }

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

    friend bool operator!=(const sequence_iterator& lhs, const sequence_iterator& rhs)
    {
        return !(lhs == rhs);
    }
};

template <class T>
struct sequence : inspect_mixin<T>,
                  inspect_indexed_mixin<T>,
                  transform_mixin<T>,
                  transform_indexed_mixin<T>,
                  filter_mixin<T>,
                  filter_indexed_mixin<T>,
                  transform_maybe_mixin<T>,
                  transform_maybe_indexed_mixin<T>,
                  drop_while_mixin<T>,
                  drop_while_indexed_mixin<T>,
                  take_while_mixin<T>,
                  take_while_indexed_mixin<T>,
                  drop_mixin<T>,
                  take_mixin<T>,
                  step_mixin<T>,
                  join_mixin<T>,
                  for_each_mixin<T>,
                  for_each_indexed_mixin<T>
{
    using iterator = sequence_iterator<T>;
    using next_function_type = typename iterator::next_function_type;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer = typename iterator::pointer;
    using difference_type = typename iterator::difference_type;

    next_function_type m_next_fn;

    explicit sequence(next_function_type next_fn) : m_next_fn(std::move(next_fn))
    {
    }

    template <class U, std::enable_if_t<std::is_constructible_v<reference, U>, int> = 0>
    sequence(const sequence<U>& other) : sequence(cast_sequence<reference, U>{ other.get_next_function() })
    {
    }

    template <class U, std::enable_if_t<std::is_constructible_v<reference, U>, int> = 0>
    sequence(sequence<U>&& other) : sequence(cast_sequence<reference, U>{ std::move(other).get_next_function() })
    {
    }

    template <class Iter, std::enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>, int> = 0>
    sequence(Iter b, Iter e) : sequence(view_sequence<Iter, reference>{ b, e })
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        std::enable_if_t<std::is_constructible_v<reference, iter_reference_t<Iter>>, int> = 0>
    sequence(Range&& range) : sequence(std::begin(range), std::end(range))
    {
    }

    sequence() : sequence(empty_sequence<T>{})
    {
    }

    template <class Container, std::enable_if_t<std::is_constructible_v<Container, iterator, iterator>, int> = 0>
    operator Container() const
    {
        return Container{ begin(), end() };
    }

    auto begin() const -> iterator
    {
        return iterator{ m_next_fn };
    }

    auto end() const -> iterator
    {
        return iterator{};
    }

    auto get_next_function() const& -> const next_function_type&
    {
        return m_next_fn;
    }

    auto get_next_function() && -> next_function_type&&
    {
        return std::move(m_next_fn);
    }

    auto maybe_front() const& -> maybe<reference>
    {
        return get_next_function()();
    }

    auto maybe_front() && -> maybe<reference>
    {
        return std::move(*this).get_next_function()();
    }

    auto maybe_at(difference_type n) && -> maybe<reference>
    {
        return this->drop(n).maybe_front();
    }

    template <class Pred>
    auto find_if(Pred pred) const -> maybe<reference>
    {
        return this->drop_while(std::not_fn(std::move(pred))).maybe_front();
    }

    template <class Pred>
    auto index_of(Pred pred) const -> maybe<difference_type>
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

    template <class Output>
    auto copy(Output out) const -> Output
    {
        return std::copy(begin(), end(), std::move(out));
    }

    template <class Seed, class BinaryFunc>
    auto accumulate(Seed seed, BinaryFunc&& func) const -> Seed
    {
        return std::accumulate(begin(), end(), std::move(seed), std::forward<BinaryFunc>(func));
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
        auto operator()() const -> iteration_result_t<In>
        {
            return m_current++;
        }
    };

    template <class T>
    auto operator()(T init) const -> sequence<T>
    {
        return sequence<T>{ next_function<T>{ init } };
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
    auto operator()(T lower, T upper) const -> sequence<T>
    {
        return sequence<T>{ next_function<T>{ lower, upper } };
    }

    template <class T>
    auto operator()(T upper) const -> sequence<T>
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
    auto operator()(S state, Func&& func) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<std::decay_t<Func>, S, Out>{ std::forward<Func>(func), std::move(state) } };
    }
};

struct view_fn
{
    template <class Iter, class Out>
    struct next_function
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

    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range&& range) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<iterator_t<Range>, Out>{ std::begin(range), std::end(range) } };
    }

    template <class Iter, class Out = iter_reference_t<Iter>>
    auto operator()(Iter b, Iter e) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<Iter, Out>{ b, e } };
    }

    template <class T>
    auto operator()(const sequence<T>& seq) const -> sequence<T>
    {
        return seq;
    }

    template <class T>
    auto operator()(sequence<T>&& seq) const -> sequence<T>
    {
        return seq;
    }
};

struct owning_fn
{
    template <class Range, class Iter, class Out>
    struct next_function
    {
        std::shared_ptr<Range> m_range;
        mutable Iter m_iter;

        next_function(std::shared_ptr<Range> range) : m_range(range), m_iter(std::begin(*m_range))
        {
        }

        auto operator()() const -> maybe<Out>
        {
            if (m_iter == std::end(*m_range))
            {
                return {};
            }
            return *m_iter++;
        }
    };

    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range range) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<Range, iterator_t<Range>, Out>{ std::make_shared<Range>(std::move(range)) } };
    }

    template <class T>
    auto operator()(const sequence<T>& s) const -> sequence<T>
    {
        return s;
    }

    template <class T>
    auto operator()(sequence<T>&& s) const -> sequence<T>
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

        auto operator()() const -> iteration_result_t<const T&>
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
    auto operator()(T value) const -> sequence<const T&>
    {
        return sequence<const T&>{ next_function<T>{ std::move(value) } };
    }
};

struct repeat_fn
{
    template <class T>
    struct next_function
    {
        T m_value;

        auto operator()() const -> iteration_result_t<const T&>
        {
            return m_value;
        }
    };

    template <class T>
    auto operator()(T value) const -> sequence<const T&>
    {
        return sequence<const T&>{ next_function<T>{ std::move(value) } };
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
        const sequence<T0>& s0,
        const sequence<T1>& s1,
        const sequence<T2>& s2,
        const sequence<T3>& s3) const -> sequence<Out>
    {
        return (*this)((*this)(s0, s1, s2), s3);
    }

    template <class T0, class T1, class T2, class Out = std::common_type_t<T0, T1, T2>>
    auto operator()(  //
        const sequence<T0>& s0,
        const sequence<T1>& s1,
        const sequence<T2>& s2) const -> sequence<Out>
    {
        return (*this)((*this)(s0, s1), s2);
    }

    template <class T0, class T1, class Out = std::common_type_t<T0, T1>>
    auto operator()(  //
        const sequence<T0>& s0,
        const sequence<T1>& s1) const -> sequence<Out>
    {
        return (*this)(sequence<Out>{ s0 }, sequence<Out>{ s1 });
    }

    template <class T>
    auto operator()(  //
        const sequence<T>& lhs,
        const sequence<T>& rhs) const -> sequence<T>
    {
        return sequence<T>{ next_function<T>{ lhs.get_next_function(), rhs.get_next_function() } };
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
        const sequence<T0>& s0,
        const sequence<T1>& s1,
        const sequence<T2>& s2,
        const sequence<T3>& s3) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<T0, T1, T2, T3>{
            s0.get_next_function(), s1.get_next_function(), s2.get_next_function(), s3.get_next_function() } };
    }

    template <class T0, class T1, class T2, class Out = std::tuple<T0, T1, T2>>
    auto operator()(  //
        const sequence<T0>& s0,
        const sequence<T1>& s1,
        const sequence<T2>& s2) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<T0, T1, T2>{
            s0.get_next_function(), s1.get_next_function(), s2.get_next_function() } };
    }

    template <class T0, class T1, class Out = std::tuple<T0, T1>>
    auto operator()(  //
        const sequence<T0>& s0,
        const sequence<T1>& s1) const -> sequence<Out>
    {
        return sequence<Out>{ next_function<T0, T1>{ s0.get_next_function(), s1.get_next_function() } };
    }
};

struct vec_fn
{
    template <class T, class... Tail>
    auto operator()(T head, Tail&&... tail) const -> sequence<const T&>
    {
        return owning_fn{}(std::vector<T>{ std::move(head), std::forward<Tail>(tail)... });
    }
};

struct init_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(std::ptrdiff_t n, Func&& func) const -> sequence<Out>
    {
        return range_fn{}(n).transform(std::forward<Func>(func));
    }
};

struct init_infinite_fn
{
    template <class Func, class Out = std::invoke_result_t<Func, std::ptrdiff_t>>
    auto operator()(Func&& func) const -> sequence<Out>
    {
        return iota_fn{}(std::ptrdiff_t{ 0 }).transform(std::forward<Func>(func));
    }
};

struct get_lines_fn
{
    static std::istream& get_line(std::istream& is, std::string& str)
    {
        str.clear();

        std::istream::sentry se(is, true);
        std::streambuf* sb = is.rdbuf();

        while (true)
        {
            const int c = sb->sbumpc();
            switch (c)
            {
                case '\n': return is;
                case '\r':
                    if (sb->sgetc() == '\n')
                    {
                        sb->sbumpc();
                    }
                    return is;
                case std::streambuf::traits_type::eof():
                    if (str.empty())
                    {
                        is.setstate(std::ios::eofbit);
                    }
                    return is;
                default: str += (char)c;
            }
        }
        return is;
    }

    struct next_function
    {
        std::istream& m_is;

        auto operator()() const -> iteration_result_t<std::string>
        {
            std::string line = {};
            return get_line(m_is, line)  //
                       ? iteration_result_t<std::string>{ std::move(line) }
                       : iteration_result_t<std::string>{};
        }
    };

    auto operator()(std::istream& is) const -> sequence<std::string>
    {
        return sequence<std::string>{ next_function{ is } };
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
static constexpr inline auto get_lines = detail::get_lines_fn{};

}  // namespace seq

template <class L, class R>
auto operator+(const sequence<L>& lhs, const sequence<R>& rhs) -> sequence<std::common_type_t<L, R>>
{
    return concat(lhs, rhs);
}

struct glyph
{
    char32_t m_data;

    friend std::ostream& operator<<(std::ostream& os, const glyph& item)
    {
        std::array<char, 4> data;
        const zx::span<char> v = std::invoke(
            [&]() -> zx::span<char>
            {
                auto state = std::mbstate_t{};
                const std::uint8_t size = std::c32rtomb(data.data(), item.m_data, &state);
                if (size == std::size_t(-1))
                {
                    throw std::runtime_error{ "u32_to_mb: error in conversion" };
                }
                return zx::span<char>{ data.data(), data.data() + size };
            });
        std::copy(v.begin(), v.end(), std::ostream_iterator<char>{ os });
        return os;
    }

    static auto read(std::string_view txt) -> zx::maybe<std::pair<glyph, std::string_view>>
    {
        std::setlocale(LC_ALL, "en_US.utf8");
        std::mbstate_t state{};
        char32_t c32 = {};
        std::size_t rc = std::mbrtoc32(&c32, txt.begin(), txt.size(), &state);
        if (rc == std::size_t(-3))
        {
            throw std::runtime_error{ "u32_to_mb: error in conversion" };
        }
        if (rc == std::size_t(-1))
        {
            return {};
        }
        if (rc == std::size_t(-2))
        {
            return {};
        }
        txt.remove_prefix(rc);
        return std::pair{ glyph{ c32 }, txt };
    }

    static auto to_glyphs(std::string_view text) -> zx::sequence<glyph>
    {
        return zx::sequence<glyph>{ [=]() mutable -> zx::iteration_result_t<glyph>
                                    {
                                        if (auto n = read(text))
                                        {
                                            const auto [ch, remainder] = *n;
                                            text = remainder;
                                            return ch;
                                        }
                                        return {};
                                    } };
    }

    friend bool operator==(const glyph& lhs, const glyph& rhs)
    {
        return lhs.m_data == rhs.m_data;
    }

    friend bool operator!=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const glyph& lhs, const glyph& rhs)
    {
        return lhs.m_data < rhs.m_data;
    }

    friend bool operator>(const glyph& lhs, const glyph& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const glyph& lhs, const glyph& rhs)
    {
        return !(lhs < rhs);
    }
};

}  // namespace zx
