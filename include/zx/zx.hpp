#pragma once

/*
   __                  __
  / /    ____ __  __   \ \
 | |    |_  / \ \/ /    | |
 | |     / /   >  <     | |
 | |    /___| /_/\_\    | |
  \_\                  /_/
*/

#include <algorithm>
#include <cstdint>
#include <cuchar>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>
#ifdef __GNUG__
#include <cxxabi.h>

#include <cstdlib>
#include <memory>

#endif  // __GNUG__

namespace zx
{

/*
                                            _            _
  _ __    _   _   _ __ ___     ___   _ __  (_)   ___    | |_   _   _   _ __     ___   ___
 | '_ \  | | | | | '_ ` _ \   / _ \ | '__| | |  / __|   | __| | | | | | '_ \   / _ \ / __|
 | | | | | |_| | | | | | | | |  __/ | |    | | | (__    | |_  | |_| | | |_) | |  __/ \__ \
 |_| |_|  \__,_| |_| |_| |_|  \___| |_|    |_|  \___|    \__|  \__, | | .__/   \___| |___/
                                                               |___/  |_|
*/

using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using i16 = std::int16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;
using usize = std::size_t;
using isize = std::ptrdiff_t;

/*
  _                                    _                    _   _
 | |_   _   _   _ __     ___          | |_   _ __    __ _  (_) | |_   ___
 | __| | | | | | '_ \   / _ \         | __| | '__|  / _` | | | | __| / __|
 | |_  | |_| | | |_) | |  __/         | |_  | |    | (_| | | | | |_  \__ \
  \__|  \__, | | .__/   \___|  _____   \__| |_|     \__,_| |_|  \__| |___/
        |___/  |_|            |_____|
*/

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

template <template <class...> class Op, class... Args>
struct is_detected : detail::detector_impl<std::void_t<>, Op, Args...>
{
};

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

#define ZX_DEFINE_IS_DETECTED_1(name, ...)          \
    template <class T0>                             \
    using name##impl = decltype(__VA_ARGS__);       \
                                                    \
    template <class T0>                             \
    struct name : ::zx::is_detected<name##impl, T0> \
    {                                               \
    };                                              \
                                                    \
    template <class T0>                             \
    constexpr bool name##_v = name<T0>::value;

#define ZX_DEFINE_IS_DETECTED_2(name, ...)              \
    template <class T0, class T1>                       \
    using name##impl = decltype(__VA_ARGS__);           \
                                                        \
    template <class T0, class T1 = T0>                  \
    struct name : ::zx::is_detected<name##impl, T0, T1> \
    {                                                   \
    };                                                  \
                                                        \
    template <class T0, class T1 = T0>                  \
    constexpr bool name##_v = name<T0, T1>::value;

#define ZX_DEFINE_IS_DETECTED_3(name, ...)                  \
    template <class T0, class T1, class T2>                 \
    using name##impl = decltype(__VA_ARGS__);               \
                                                            \
    template <class T0, class T1 = T0, class T2 = T1>       \
    struct name : ::zx::is_detected<name##impl, T0, T1, T2> \
    {                                                       \
    };                                                      \
                                                            \
    template <class T0, class T1 = T0, class T2 = T1>       \
    constexpr bool name##_v = name<T0, T1, T2>::value;

ZX_DEFINE_IS_DETECTED_1(has_ostream_operator, std::declval<std::ostream>() << std::declval<T0>());

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

template <class T>
using iterator_t = decltype(std::begin(std::declval<T>()));

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

#ifdef __GNUG__

inline auto demangle(const char* name) -> std::string
{
    int status = -4;
    std::unique_ptr<char, void (*)(void*)> res{ abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
    return (status == 0) ? res.get() : name;
}

#else

// does nothing if not g++
inline auto demangle(const char* name) -> std::string
{
    return name;
}

#endif  // __GNUG__

template <class T>
struct type
{
    friend std::ostream& operator<<(std::ostream& os, const type&)
    {
        return os << demangle(typeid(T).name());
    }
};

template <class T>
constexpr auto type_of(const T&) -> type<T>
{
    return type<T>{};
}

/*
   __                                      _     _     _
  / _|   ___    _ __   _ __ ___     __ _  | |_  | |_  (_)  _ __     __ _
 | |_   / _ \  | '__| | '_ ` _ \   / _` | | __| | __| | | | '_ \   / _` |
 |  _| | (_) | | |    | | | | | | | (_| | | |_  | |_  | | | | | | | (_| |
 |_|    \___/  |_|    |_| |_| |_|  \__,_|  \__|  \__| |_| |_| |_|  \__, |
                                                                   |___/
*/

struct ostream_writer : public std::function<void(std::ostream&)>
{
    using base_type = std::function<void(std::ostream&)>;
    using base_type::base_type;

    operator std::string() const
    {
        std::stringstream ss;
        (*this)(ss);
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const ostream_writer& item)
    {
        item(os);
        return os;
    }
};

template <class T, class = void>
struct formatter;

ZX_DEFINE_IS_DETECTED_1(has_formatter, formatter<T0>{})

namespace detail
{

static constexpr inline struct format_to_fn
{
    template <class T>
    static void do_format(std::ostream& os, const T& item)
    {
        if constexpr (has_formatter_v<T>)
        {
            formatter<T>{}.format(os, item);
        }
        else if constexpr (has_ostream_operator_v<T>)
        {
            os << item;
        }
        else
        {
            os << "[" << type_of(item) << "]";
        }
    }

    template <class... Args>
    auto operator()(std::ostream& os, Args&&... args) const -> std::ostream&
    {
        (do_format(os, std::forward<Args>(args)), ...);
        return os;
    }
} format_to;

static constexpr inline struct format_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::string
    {
        std::stringstream ss;
        format_to(ss, std::forward<Args>(args)...);
        return std::move(ss).str();
    }
} format;

static constexpr inline struct print_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::ostream&
    {
        return format_to(std::cout, std::forward<Args>(args)...);
    }
} print;

static constexpr inline struct println_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::ostream&
    {
        return print(std::forward<Args>(args)...) << std::endl;
    }
} println;

static constexpr inline struct delimit_fn
{
    template <class Iter>
    auto operator()(Iter begin, Iter end, std::string_view separator) const -> ostream_writer
    {
        return [=](std::ostream& os)
        {
            if (begin == end)
            {
                return;
            }

            format_to(os, *begin);
            for (auto it = std::next(begin); it != end; ++it)
            {
                format_to(os, separator, *it);
            }
        };
    }

    template <class Range>
    auto operator()(Range&& range, std::string_view separator) const -> ostream_writer
    {
        return (*this)(std::begin(range), std::end(range), separator);
    }

    template <class T>
    auto operator()(std::initializer_list<T> range, std::string_view separator) const -> ostream_writer
    {
        return (*this)(std::begin(range), std::end(range), separator);
    }
} delimit;

}  // namespace detail

using detail::delimit;
using detail::format;
using detail::format_to;
using detail::print;
using detail::println;
static constexpr inline auto str = format;

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
                os, "std::exception_ptr<", demangle(abi::__cxa_current_exception_type()->name()), ">(", ex.what(), ")");
#else
            format_to(os, "std::exception_ptr<std::exception>(", ex.what(), ")");
#endif  // __GNUG__
        }

        catch (const std::string& ex)
        {
            format_to(os, "std::exception_ptr<std::string>(", ex, ")");
        }
        catch (const char* ex)
        {
            format_to(os, "std::exception_ptr<const char*>(", ex, ")");
        }
        catch (int ex)
        {
            format_to(os, "std::exception_ptr<int>(", ex, ")");
        }
        catch (...)
        {
            format_to(os, "std::exception_ptr<...>(...)");
        }
    }
};

template <>
struct formatter<bool>
{
    void format(std::ostream& os, const bool item) const
    {
        os << std::boolalpha << item;
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
                                        _                           _   _   _
   ___   _ __   _ __    ___    _ __    | |__     __ _   _ __     __| | | | (_)  _ __     __ _
  / _ \ | '__| | '__|  / _ \  | '__|   | '_ \   / _` | | '_ \   / _` | | | | | | '_ \   / _` |
 |  __/ | |    | |    | (_) | | |      | | | | | (_| | | | | | | (_| | | | | | | | | | | (_| |
  \___| |_|    |_|     \___/  |_|      |_| |_|  \__,_| |_| |_|  \__,_| |_| |_| |_| |_|  \__, |
                                                                                        |___/
*/

class source_location
{
public:
    constexpr source_location(std::string_view file_name, u32 line, std::string_view function_name)
        : m_file_name{ file_name }
        , m_line{ line }
        , m_function_name{ function_name }
    {
    }

    constexpr source_location(const source_location&) = default;
    constexpr source_location(source_location&&) noexcept = default;

    constexpr std::string_view file_name() const noexcept
    {
        return m_file_name;
    }

    constexpr std::string_view function_name() const noexcept
    {
        return m_function_name;
    }

    constexpr u32 line() const noexcept
    {
        return m_line;
    }

    friend std::ostream& operator<<(std::ostream& os, const source_location& item)
    {
        return format_to(os, item.file_name(), '(', item.line(), ')', ": ", item.function_name());
    }

private:
    std::string_view m_file_name;
    u32 m_line;
    std::string_view m_function_name;
};

template <class E = std::runtime_error, class... Args>
void assert_that(bool condition, Args&&... args)
{
    if (!condition)
    {
        throw E{ str(std::forward<Args>(args)...) };
    }
}

/*
                 _   _     _                          _     _            _
   __ _   _ __  (_) | |_  | |__    _ __ ___     ___  | |_  (_)   ___    | |_   _   _   _ __     ___   ___
  / _` | | '__| | | | __| | '_ \  | '_ ` _ \   / _ \ | __| | |  / __|   | __| | | | | | '_ \   / _ \ / __|
 | (_| | | |    | | | |_  | | | | | | | | | | |  __/ | |_  | | | (__    | |_  | |_| | | |_) | |  __/ \__ \
  \__,_| |_|    |_|  \__| |_| |_| |_| |_| |_|  \___|  \__| |_|  \___|    \__|  \__, | | .__/   \___| |___/
                                                                               |___/  |_|
*/

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

struct bad_result_access : std::runtime_error
{
    explicit bad_result_access(const std::string& msg) : std::runtime_error{ msg }
    {
    }
};

/*
                              _   _      __  _____         _____  __
  _ __    ___   ___   _   _  | | | |_   / / |_   _|       | ____| \ \
 | '__|  / _ \ / __| | | | | | | | __| / /    | |         |  _|    \ \
 | |    |  __/ \__ \ | |_| | | | | |_  \ \    | |    _    | |___   / /
 |_|     \___| |___/  \__,_| |_|  \__|  \_\   |_|   ( )   |_____| /_/
                                                    |/
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

    constexpr const value_type& operator*() const&
    {
        assert_that<bad_result_access>(has_value(), "accessing the value of an empty 'result' object");
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type& operator*() &
    {
        assert_that<bad_result_access>(has_value(), "accessing the value of an empty 'result' object");
        return std::get<value_storage>(m_storage);
    }

    constexpr value_type&& operator*() &&
    {
        assert_that<bad_result_access>(has_value(), "accessing the value of an empty 'result' object");
        return std::get<value_storage>(std::move(m_storage));
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
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
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

    template <class U>
    constexpr auto value_or(U&& v) const& -> value_type
    {
        return *this ? **this : static_cast<value_type>(std::forward<U>(v));
    }

    template <class U>
    constexpr auto value_or(U&& v) && -> value_type
    {
        return *this ? *std::move(*this) : static_cast<value_type>(std::forward<U>(v));
    }

private:
    std::variant<value_storage, error_storage> m_storage;
};

/*
                              _   _      __  _____    ___           _____  __
  _ __    ___   ___   _   _  | | | |_   / / |_   _|  ( _ )         | ____| \ \
 | '__|  / _ \ / __| | | | | | | | __| / /    | |    / _ \/\       |  _|    \ \
 | |    |  __/ \__ \ | |_| | | | | |_  \ \    | |   | (_>  <  _    | |___   / /
 |_|     \___| |___/  \__,_| |_|  \__|  \_\   |_|    \___/\/ ( )   |_____| /_/
                                                             |/
*/

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
        assert_that<bad_result_access>(has_value(), "accessing the value of an empty 'result' object");
        return std::get<value_storage>(m_storage);
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
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type& error() &
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return std::get<error_storage>(m_storage).m_error;
    }

    constexpr error_type&& error() &&
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
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

    template <class U>
    constexpr auto value_or(U&& v) const& -> value_type
    {
        return *this ? **this : static_cast<value_type>(std::forward<U>(v));
    }

    template <class U>
    constexpr auto value_or(U&& v) && -> value_type
    {
        return *this ? *std::move(*this) : static_cast<value_type>(std::forward<U>(v));
    }

private:
    std::variant<value_storage, error_storage> m_storage;
};

/*
                              _   _      __                  _       _         _____  __
  _ __    ___   ___   _   _  | | | |_   / / __   __   ___   (_)   __| |       | ____| \ \
 | '__|  / _ \ / __| | | | | | | | __| / /  \ \ / /  / _ \  | |  / _` |       |  _|    \ \
 | |    |  __/ \__ \ | |_| | | | | |_  \ \   \ V /  | (_) | | | | (_| |  _    | |___   / /
 |_|     \___| |___/  \__,_| |_|  \__|  \_\   \_/    \___/  |_|  \__,_| ( )   |_____| /_/
                                                                        |/
*/

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
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return *m_storage;
    }

    constexpr error_type& error() &
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
        return *m_storage;
    }

    constexpr error_type&& error() &&
    {
        assert_that<bad_result_access>(has_error(), "accessing the error of a 'result' object with value");
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
std::ostream& operator<<(std::ostream& os, const result<void, E>& item)
{
    return item.has_value() ? format_to(os, "ok()") : format_to(os, "error(", item.error(), ")");
}

template <class T, class E>
std::ostream& operator<<(std::ostream& os, const result<T, E>& item)
{
    return item.has_value() ? format_to(os, "ok(", item.value(), ")") : format_to(os, "error(", item.error(), ")");
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

struct bad_maybe_access : std::runtime_error
{
    explicit bad_maybe_access(const std::string& msg) : std::runtime_error{ msg }
    {
    }
};

/*
                              _               __  _____  __
  _ __ ___     __ _   _   _  | |__     ___   / / |_   _| \ \
 | '_ ` _ \   / _` | | | | | | '_ \   / _ \ / /    | |    \ \
 | | | | | | | (_| | | |_| | | |_) | |  __/ \ \    | |    / /
 |_| |_| |_|  \__,_|  \__, | |_.__/   \___|  \_\   |_|   /_/
                      |___/
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

    constexpr const value_type& operator*() const&
    {
        assert_that<bad_maybe_access>(has_value(), "accessing value of an empty 'maybe' object");
        return *m_storage;
    }

    constexpr value_type& operator*() &
    {
        assert_that<bad_maybe_access>(has_value(), "accessing value of an empty 'maybe' object");
        return *m_storage;
    }

    constexpr value_type&& operator*() &&
    {
        assert_that<bad_maybe_access>(has_value(), "accessing value of an empty 'maybe' object");
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

    template <class U>
    constexpr auto value_or(U&& v) const& -> value_type
    {
        return *this ? **this : static_cast<value_type>(std::forward<U>(v));
    }

    template <class U>
    constexpr auto value_or(U&& v) && -> value_type
    {
        return *this ? *std::move(*this) : static_cast<value_type>(std::forward<U>(v));
    }

private:
    std::optional<value_type> m_storage;
};

/*
                              _               __  _____    ___    __
  _ __ ___     __ _   _   _  | |__     ___   / / |_   _|  ( _ )   \ \
 | '_ ` _ \   / _` | | | | | | '_ \   / _ \ / /    | |    / _ \/\  \ \
 | | | | | | | (_| | | |_| | | |_) | |  __/ \ \    | |   | (_>  <  / /
 |_| |_| |_|  \__,_|  \__, | |_.__/   \___|  \_\   |_|    \___/\/ /_/
                      |___/
*/

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
        assert_that<bad_maybe_access>(has_value(), "accessing value of an empty 'maybe' object");
        return *m_storage;
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

    template <class U>
    constexpr auto value_or(U&& v) const -> value_type
    {
        return *this ? **this : static_cast<value_type>(std::forward<U>(v));
    }

private:
    value_type* m_storage;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const maybe<T>& item)
{
    return item.has_value() ? format_to(os, "some(", item.value(), ")") : format_to(os, "none");
}

/*
  _   _                           _                            _           _                    __
 (_) | |_    ___   _ __    __ _  | |_    ___    _ __          (_)  _ __   | |_    ___   _ __   / _|   __ _    ___    ___
 | | | __|  / _ \ | '__|  / _` | | __|  / _ \  | '__|         | | | '_ \  | __|  / _ \ | '__| | |_   / _` |  / __|  / _ \
 | | | |_  |  __/ | |    | (_| | | |_  | (_) | | |            | | | | | | | |_  |  __/ | |    |  _| | (_| | | (__  |  __/
 |_|  \__|  \___| |_|     \__,_|  \__|  \___/  |_|     _____  |_| |_| |_|  \__|  \___| |_|    |_|    \__,_|  \___|  \___|
                                                      |_____|
*/

namespace detail
{

template <template <class> class C>
struct convertible_to
{
    template <class T, class = std::enable_if_t<C<T>::value>>
    operator T() const;
};

ZX_DEFINE_IS_DETECTED_1(iter_has_deref, std::declval<T0>().deref());
ZX_DEFINE_IS_DETECTED_1(iter_has_inc, std::declval<T0>().inc());
ZX_DEFINE_IS_DETECTED_1(iter_has_dec, std::declval<T0>().dec());
ZX_DEFINE_IS_DETECTED_1(iter_has_advance, std::declval<T0>().advance(std::declval<convertible_to<std::is_integral>>()));
ZX_DEFINE_IS_DETECTED_1(iter_has_is_equal, std::declval<T0>().is_equal(std::declval<T0>()));
ZX_DEFINE_IS_DETECTED_1(iter_has_is_less, std::declval<T0>().is_less(std::declval<T0>()));
ZX_DEFINE_IS_DETECTED_1(iter_has_distance_to, std::declval<T0>().distance_to(std::declval<T0>()));

template <class T, class = std::void_t<>>
struct difference_type_impl
{
    using type = std::ptrdiff_t;
};

template <class T>
struct difference_type_impl<T, std::enable_if_t<iter_has_distance_to_v<T>>>
{
    using type = decltype(std::declval<T>().distance_to(std::declval<T>()));
};

template <class T>
constexpr bool iter_is_incrementable_v = iter_has_advance_v<T> || iter_has_inc_v<T>;

template <class T>
constexpr bool iter_is_decrementable_v = iter_has_advance_v<T> || iter_has_dec_v<T>;

template <class T>
constexpr bool iter_is_equality_comparable_v = iter_has_is_equal_v<T> || iter_has_distance_to_v<T>;

template <class T>
constexpr bool iter_is_less_than_comparable_v = iter_has_is_less_v<T> || iter_has_distance_to_v<T>;

template <class T>
using difference_type = typename detail::difference_type_impl<T>::type;

template <class T>
struct pointer_proxy
{
    T item;

    T* operator->()
    {
        return std::addressof(item);
    }
};

template <class Impl>
struct iterator_interface
{
    Impl m_impl;

    template <class... Args, require<std::is_constructible<Impl, Args...>{}> = 0>
    iterator_interface(Args&&... args) : m_impl{ std::forward<Args>(args)... }
    {
    }

    iterator_interface() = default;
    iterator_interface(const iterator_interface&) = default;
    iterator_interface(iterator_interface&&) = default;

    iterator_interface& operator=(iterator_interface other)
    {
        std::swap(m_impl, other.m_impl);
        return *this;
    }

    static_assert(iter_has_deref_v<Impl>, ".deref required");
    static_assert(std::is_default_constructible<Impl>{}, "iterator_interface: default constructible Impl required");

    using reference = decltype(m_impl.deref());

private:
    template <class R = reference, require<std::is_reference<R>{}> = 0>
    auto get_pointer() const -> std::add_pointer_t<reference>
    {
        return std::addressof(**this);
    }

    template <class R = reference, require<!std::is_reference<R>{}> = 0>
    auto get_pointer() const -> pointer_proxy<reference>
    {
        return { **this };
    }

    template <class Impl_ = Impl, require<iter_is_incrementable_v<Impl_>> = 0>
    void inc()
    {
        if constexpr (iter_has_inc_v<Impl_>)
        {
            m_impl.inc();
        }
        else
        {
            m_impl.advance(+1);
        }
    }

    template <class Impl_ = Impl, require<iter_is_decrementable_v<Impl_>> = 0>
    void dec()
    {
        if constexpr (iter_has_dec_v<Impl_>)
        {
            m_impl.dec();
        }
        else
        {
            m_impl.advance(-1);
        }
    }

    template <class Impl_ = Impl, require<iter_is_equality_comparable_v<Impl_>> = 0>
    bool is_equal(const Impl& other) const
    {
        if constexpr (iter_has_is_equal_v<Impl_>)
        {
            return m_impl.is_equal(other);
        }
        else
        {
            return m_impl.distance_to(other) == 0;
        }
    }

    template <class Impl_ = Impl, require<iter_is_less_than_comparable_v<Impl_>> = 0>
    bool is_less(const Impl& other) const
    {
        if constexpr (iter_has_is_less_v<Impl_>)
        {
            return m_impl.is_less(other);
        }
        else
        {
            return m_impl.distance_to(other) > 0;
        }
    }

public:
    reference operator*() const
    {
        return m_impl.deref();
    }

    auto operator->() const -> decltype(get_pointer())
    {
        return get_pointer();
    }

    template <class Impl_ = Impl, require<iter_is_incrementable_v<Impl_>> = 0>
    iterator_interface& operator++()
    {
        inc();
        return *this;
    }

    template <class Impl_ = Impl, require<iter_is_incrementable_v<Impl_>> = 0>
    iterator_interface operator++(int)
    {
        iterator_interface tmp{ *this };
        ++(*this);
        return tmp;
    }

    template <class Impl_ = Impl, require<iter_is_decrementable_v<Impl_>> = 0>
    iterator_interface& operator--()
    {
        dec();
        return *this;
    }

    template <class Impl_ = Impl, require<iter_is_decrementable_v<Impl_>> = 0>
    iterator_interface operator--(int)
    {
        iterator_interface tmp{ *this };
        --(*this);
        return tmp;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
    friend iterator_interface& operator+=(iterator_interface& it, D offset)
    {
        it.m_impl.advance(offset);
        return it;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
    friend iterator_interface operator+(iterator_interface it, D offset)
    {
        return it += offset;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
    friend iterator_interface operator+(D offset, iterator_interface it)
    {
        return it + offset;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
    friend iterator_interface& operator-=(iterator_interface& it, D offset)
    {
        return it += -offset;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
    friend iterator_interface operator-(iterator_interface it, D offset)
    {
        return it -= offset;
    }

    template <class D, class Impl_ = Impl, require<iter_has_advance_v<Impl_> && std::is_integral_v<D>> = 0>
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

    template <class Impl_ = Impl, require<iter_is_less_than_comparable_v<Impl_>> = 0>
    friend bool operator<(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return lhs.is_less(rhs.m_impl);
    }

    template <class Impl_ = Impl, require<iter_is_less_than_comparable_v<Impl_>> = 0>
    friend bool operator>(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return rhs < lhs;
    }

    template <class Impl_ = Impl, require<iter_is_less_than_comparable_v<Impl_>> = 0>
    friend bool operator<=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs > rhs);
    }

    template <class Impl_ = Impl, require<iter_is_less_than_comparable_v<Impl_>> = 0>
    friend bool operator>=(const iterator_interface& lhs, const iterator_interface& rhs)
    {
        return !(lhs < rhs);
    }

    template <class Impl_ = Impl, require<iter_has_distance_to_v<Impl_>> = 0>
    friend auto operator-(const iterator_interface& lhs, const iterator_interface& rhs) -> difference_type<Impl_>
    {
        return rhs.m_impl.distance_to(lhs.m_impl);
    }
};

template <class T, class = std::void_t<>>
struct iterator_category_impl
{
    using type = std::conditional_t<
        iter_has_advance_v<T> && iter_has_distance_to_v<T>,
        std::random_access_iterator_tag,
        std::conditional_t<
            iter_has_dec_v<T> || iter_has_advance_v<T>,
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

/*
  _   _                           _                                                                   __  _____  __
 (_) | |_    ___   _ __    __ _  | |_    ___    _ __           _ __    __ _   _ __     __ _    ___   / / |_   _| \ \
 | | | __|  / _ \ | '__|  / _` | | __|  / _ \  | '__|         | '__|  / _` | | '_ \   / _` |  / _ \ / /    | |    \ \
 | | | |_  |  __/ | |    | (_| | | |_  | (_) | | |            | |    | (_| | | | | | | (_| | |  __/ \ \    | |    / /
 |_|  \__|  \___| |_|     \__,_|  \__|  \___/  |_|     _____  |_|     \__,_| |_| |_|  \__, |  \___|  \_\   |_|   /_/
                                                      |_____|                         |___/
*/

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

template <class Iter>
struct iterator_range_base
{
    using iterator = Iter;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr iterator_range_base() = default;
    constexpr iterator_range_base(const iterator_range_base&) = default;
    constexpr iterator_range_base(iterator_range_base&&) noexcept = default;

    constexpr iterator_range_base(iterator b, iterator e) : m_iterators(b, e)
    {
    }

    constexpr iterator_range_base(const std::pair<iterator, iterator>& pair) : m_iterators(pair)
    {
    }

    constexpr iterator_range_base(iterator b, difference_type n) : iterator_range_base(b, std::next(b, n))
    {
    }

    template <class Range, class It = iterator_t<Range>, require<std::is_constructible_v<iterator, It>> = 0>
    constexpr iterator_range_base(Range&& range) : iterator_range_base(std::begin(range), std::end(range))
    {
    }

    constexpr auto begin() const noexcept -> iterator
    {
        return m_iterators.first;
    }

    constexpr auto end() const noexcept -> iterator
    {
        return m_iterators.second;
    }

    void swap(iterator_range_base& other) noexcept
    {
        std::swap(m_iterators, other.m_iterators);
    }

    std::pair<iterator, iterator> m_iterators;
};

template <class T>
struct iterator_range_base<T*>
{
    using iterator = T*;
    using pointer = iterator;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;

    constexpr iterator_range_base() = default;
    constexpr iterator_range_base(const iterator_range_base&) = default;
    constexpr iterator_range_base(iterator_range_base&&) noexcept = default;

    constexpr iterator_range_base(iterator b, iterator e) : m_iterators(b, e)
    {
    }

    constexpr iterator_range_base(const std::pair<iterator, iterator>& pair) : m_iterators(pair)
    {
    }

    constexpr iterator_range_base(iterator b, difference_type n) : iterator_range_base(b, std::next(b, n))
    {
    }

    template <class Range, class It = iterator_t<Range>, require<std::is_constructible_v<iterator, It>> = 0>
    constexpr iterator_range_base(Range&& range) : iterator_range_base(std::begin(range), std::end(range))
    {
    }

    template <
        class Range,
        class It = iterator_t<Range>,
        class Ptr = decltype(std::declval<Range>().data()),
        require<std::is_constructible_v<iterator, Ptr> && !std::is_constructible_v<iterator, It>> = 0>
    constexpr iterator_range_base(Range&& range)
        : iterator_range_base(range.data(), static_cast<difference_type>(range.size()))
    {
    }

    constexpr auto begin() const noexcept -> iterator
    {
        return m_iterators.first;
    }

    constexpr auto end() const noexcept -> iterator
    {
        return m_iterators.second;
    }

    constexpr auto data() const noexcept -> pointer
    {
        return begin();
    }

    void swap(iterator_range_base& other) noexcept
    {
        std::swap(m_iterators, other.m_iterators);
    }

    std::pair<iterator, iterator> m_iterators;
};

}  // namespace detail

struct slice_t
{
    maybe<std::ptrdiff_t> begin;
    maybe<std::ptrdiff_t> end;
};

template <class Iter>
struct iterator_range : detail::iterator_range_base<Iter>
{
    using base_t = detail::iterator_range_base<Iter>;

    using iterator = typename base_t::iterator;
    using difference_type = typename base_t::difference_type;
    using size_type = difference_type;
    using const_iterator = iterator;

    using reference = typename std::iterator_traits<iterator>::reference;

    using maybe_reference = maybe<reference>;

    using reverse_type = iterator_range<detail::reverse_iterator_t<iterator>>;

    using base_t::base_t;
    using base_t::begin;
    using base_t::end;

    iterator_range& operator=(iterator_range other) noexcept
    {
        base_t::swap(other);
        return *this;
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
        assert_that<std::out_of_range>(!empty(), "iterator_range::front - empty range");
        return *begin();
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto back() const -> reference
    {
        assert_that<std::out_of_range>(!empty(), "iterator_range::back - empty range");
        return *std::prev(end());
    }

    auto maybe_front() const -> maybe_reference
    {
        return !empty() ? maybe_reference{ *begin() } : maybe_reference{};
    }

    template <class It = iterator, require<is_bidirectional_iterator<It>::value> = 0>
    auto maybe_back() const -> maybe_reference
    {
        return !empty() ? maybe_reference{ *std::prev(end()) } : maybe_reference{};
    }

    template <class It = iterator, require<is_random_access_iterator<It>::value> = 0>
    auto at(difference_type n) const -> reference
    {
        assert_that<std::out_of_range>(0 <= n && n < size(), "iterator_range::at - index out of range");
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
        return 0 <= n && n < size() ? maybe_reference{ *std::next(begin(), n) } : maybe_reference{};
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
        static const auto adjust = [](difference_type index,
                                      size_type size) -> size_type {  //
            return std::clamp<size_type>(index >= 0 ? index : index + size, 0, size);
        };
        const size_type s = size();
        const size_type b = info.begin ? adjust(*info.begin, s) : size_type{ 0 };
        const size_type e = info.end ? adjust(*info.end, s) : s;
        return iterator_range{ begin() + b, std::max(size_type{ 0 }, e - b) };
    }

private:
    auto advance(difference_type n) const -> iterator
    {
        if constexpr (is_random_access_iterator<iterator>::value)
        {
            return begin() + std::min(ssize(), n);
        }
        else
        {
            iterator it = begin();
            while (it != end() && n > 0)
            {
                --n;
                ++it;
            }
            return it;
        }
    }
};

template <class Container>
using subrange = iterator_range<iterator_t<Container>>;

template <class T>
using span = iterator_range<const T*>;

template <class T>
using mut_span = iterator_range<T*>;

template <class T>
struct numeric_iter
{
    T m_value;

    numeric_iter(T value = {}) : m_value(value)
    {
    }

    T deref() const
    {
        return m_value;
    }

    void advance(std::ptrdiff_t n)
    {
        m_value += n;
    }

    std::ptrdiff_t distance_to(const numeric_iter& other) const
    {
        return other.m_value - m_value;
    }
};

template <class T>
using numeric_iterator = iterator_interface<numeric_iter<T>>;

template <class T>
auto range(T lo, T up) -> iterator_range<numeric_iterator<T>>
{
    return { numeric_iterator<T>{ lo }, numeric_iterator<T>{ std::max(lo, up) } };
}

template <class T>
auto range(T up) -> iterator_range<numeric_iterator<T>>
{
    return range(T{}, up);
}

template <class T = std::ptrdiff_t>
auto iota(T init = {}) -> iterator_range<numeric_iterator<T>>
{
    return range(init, std::numeric_limits<T>::max());
}

/*
   __                          _     _                           _
  / _|  _   _   _ __     ___  | |_  (_)   ___    _ __     __ _  | |
 | |_  | | | | | '_ \   / __| | __| | |  / _ \  | '_ \   / _` | | |
 |  _| | |_| | | | | | | (__  | |_  | | | (_) | | | | | | (_| | | |
 |_|    \__,_| |_| |_|  \___|  \__| |_|  \___/  |_| |_|  \__,_| |_|

*/

struct identity
{
    template <class T>
    constexpr T&& operator()(T&& item) const noexcept
    {
        return std::forward<T>(item);
    }
};

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

    template <std::size_t I, class... Args, require<(I + 1) == sizeof...(Pipes)> = 0>
    constexpr auto call(Args&&... args) const -> decltype(invoke<I>(std::forward<Args>(args)...))
    {
        return invoke<I>(std::forward<Args>(args)...);
    }

    template <std::size_t I, class... Args, require<(I + 1) < sizeof...(Pipes)> = 0>
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

template <class... Pipes>
pipe_t(Pipes&&...) -> pipe_t<std::decay_t<Pipes>...>;

template <class T>
struct is_pipeline : std::false_type
{
};

template <class... Args>
struct is_pipeline<pipe_t<Args...>> : std::true_type
{
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

static constexpr inline auto pipe = detail::pipe_fn{};

template <class... L, class... R>
constexpr auto operator|=(pipe_t<L...> lhs, pipe_t<R...> rhs) -> decltype(pipe(std::move(lhs), std::move(rhs)))
{
    return pipe(std::move(lhs), std::move(rhs));
}

template <class T, class... Pipes, require<!is_pipeline<std::decay_t<T>>{}> = 0>
constexpr auto operator|=(T&& item, const pipe_t<Pipes...>& p) -> decltype(p(std::forward<T>(item)))
{
    return p(std::forward<T>(item));
}

template <class... Pipes>
std::ostream& operator<<(std::ostream& os, const pipe_t<Pipes...>& item)
{
    format_to(os, "(pipe");
    std::apply([&os](const auto&... args) { (format_to(os, ' ', args), ...); }, item.m_pipes);
    return format_to(os, ")");
}

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

static constexpr inline struct destruct_fn
{
    template <class Func>
    struct impl
    {
        Func m_func;

        template <class Tuple>
        auto operator()(Tuple&& tuple) const -> decltype(std::apply(m_func, std::forward<Tuple>(tuple)))
        {
            return std::apply(m_func, std::forward<Tuple>(tuple));
        }
    };

    template <class Func>
    auto operator()(Func&& func) const -> pipe_t<impl<std::decay_t<Func>>>
    {
        return { { std::forward<Func>(func) } };
    }
} destruct;

template <std::size_t N>
struct get_element_fn
{
    template <class T>
    constexpr auto operator()(T&& item) const -> decltype(std::get<N>(std::forward<T>(item)))
    {
        return std::get<N>(std::forward<T>(item));
    }

    friend std::ostream& operator<<(std::ostream& os, const get_element_fn&)
    {
        return format_to(os, "(get_element ", N, ")");
    }
};

static constexpr inline struct eq_fn
{
    template <class Iter>
    bool operator()(const Iter& lhs, const Iter& rhs) const
    {
        return call(lhs, rhs, std::make_index_sequence<std::tuple_size_v<Iter>>{});
    }

private:
    template <class Iter, std::size_t... I>
    static bool call(const Iter& lhs, const Iter& rhs, std::index_sequence<I...>)
    {
        return (... || (std::get<I>(lhs) == std::get<I>(rhs)));
    }
} eq;

template <class Iter>
void inc(Iter& iter)
{
    std::apply([](auto&... it) { (++it, ...); }, iter);
}

static constexpr inline struct invoke_reducer_fn
{
    template <class Reducer, class State, class Iter>
    auto operator()(Reducer&& reducer, State state, Iter it) const -> State
    {
        return call(
            std::forward<Reducer>(reducer), std::move(state), it, std::make_index_sequence<std::tuple_size_v<Iter>>{});
    }

private:
    template <class Reducer, class State, class Iter, std::size_t... I>
    static auto call(Reducer&& reducer, State state, Iter it, std::index_sequence<I...>) -> State
    {
        return std::invoke(std::forward<Reducer>(reducer), std::move(state), *std::get<I>(it)...);
    }
} invoke_reducer;

struct reduce_fn
{
    template <class State, class Reducer>
    struct proxy_t
    {
        State m_state;
        Reducer m_reducer;

        template <class... Ranges>
        auto operator()(Ranges&&... ranges) const -> State
        {
            State state = m_state;
            const auto begin = std::tuple{ std::begin(ranges)... };
            const auto end = std::tuple{ std::end(ranges)... };
            for (auto it = begin; !eq(it, end); inc(it))
            {
                state = invoke_reducer(m_reducer, std::move(state), it);
            }
            return state;
        }
    };

    template <class State, class Reducer>
    constexpr auto operator()(State state, Reducer&& reducer) const -> proxy_t<State, std::decay_t<Reducer>>
    {
        return { std::move(state), std::forward<Reducer>(reducer) };
    }
};

struct let_fn
{
    template <class Func>
    constexpr auto operator()(Func&& func) const -> std::invoke_result_t<Func>
    {
        return std::invoke(std::forward<Func>(func));
    }

    template <class T0, class Func>
    constexpr auto operator()(T0&& t0, Func&& func) const -> std::invoke_result_t<Func, T0>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0));
    }

    template <class T0, class T1, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, Func&& func) const -> std::invoke_result_t<Func, T0, T1>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0), std::forward<T1>(t1));
    }

    template <class T0, class T1, class T2, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, Func&& func) const -> std::invoke_result_t<Func, T0, T1, T2>
    {
        return std::invoke(std::forward<Func>(func), std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2));
    }

    template <class T0, class T1, class T2, class T3, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3));
    }

    template <class T0, class T1, class T2, class T3, class T4, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3, T4>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3),
            std::forward<T4>(t4));
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class Func>
    constexpr auto operator()(T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, Func&& func) const
        -> std::invoke_result_t<Func, T0, T1, T2, T3, T4, T5>
    {
        return std::invoke(
            std::forward<Func>(func),
            std::forward<T0>(t0),
            std::forward<T1>(t1),
            std::forward<T2>(t2),
            std::forward<T3>(t3),
            std::forward<T4>(t4),
            std::forward<T5>(t5));
    }
};

}  // namespace detail

using detail::apply;
using detail::destruct;
using detail::do_all;
using detail::pipe;
using detail::with;

template <std::size_t I>
static constexpr inline auto get_element = detail::get_element_fn<I>{};

static constexpr inline auto first = get_element<0>;
static constexpr inline auto second = get_element<1>;
static constexpr inline auto key = get_element<0>;
static constexpr inline auto value = get_element<1>;

static constexpr inline auto reduce = detail::reduce_fn{};
static constexpr inline auto let = detail::let_fn{};

/*
                                                             __  _____  __
  ___    ___    __ _   _   _    ___   _ __     ___    ___   / / |_   _| \ \
 / __|  / _ \  / _` | | | | |  / _ \ | '_ \   / __|  / _ \ / /    | |    \ \
 \__ \ |  __/ | (_| | | |_| | |  __/ | | | | | (__  |  __/ \ \    | |    / /
 |___/  \___|  \__, |  \__,_|  \___| |_| |_|  \___|  \___|  \_\   |_|   /_/
                  |_|
*/

template <class T>
using iteration_result_t = maybe<T>;

template <class T>
using next_function_t = std::function<iteration_result_t<T>()>;

template <class T>
struct sequence;

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
struct associate_mixin
{
    template <class Func, class Out>
    struct next_function
    {
        Func m_func;
        next_function_t<T> m_next;

        auto operator()() const -> iteration_result_t<std::pair<T, Out>>
        {
            iteration_result_t<T> next = m_next();
            if (next)
            {
                return std::pair<T, Out>{ *next, std::invoke(m_func, *next) };
            }
            return {};
        }
    };

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto associate(Func&& func) const& -> sequence<std::pair<T, Res>>
    {
        return sequence<std::pair<T, Res>>{ next_function<std::decay_t<Func>, Res>{
            std::forward<Func>(func), static_cast<const sequence<T>&>(*this).get_next_function() } };
    }

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    auto associate(Func&& func) && -> sequence<std::pair<T, Res>>
    {
        return sequence<std::pair<T, Res>>{ next_function<std::decay_t<Func>, Res>{
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
    auto intersperse(E element) const& -> sequence<Out>
    {
        return sequence<Out>{ next_function<E, Out>{ static_cast<const sequence<T>&>(*this).get_next_function(),
                                                     std::move(element) } };
    }

    template <class E, class Out = std::common_type_t<T, E>>
    auto intersperse(E element) && -> sequence<Out>
    {
        return sequence<Out>{ next_function<E, Out>{ static_cast<sequence<T>&&>(*this).get_next_function(),
                                                     std::move(element) } };
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

template <class Range, class Iter, class Out>
struct owning_sequence
{
    std::shared_ptr<Range> m_range;
    mutable Iter m_iter;

    explicit owning_sequence(std::shared_ptr<Range> range) : m_range(range), m_iter(std::begin(*m_range))
    {
    }

    auto operator()() const -> iteration_result_t<Out>
    {
        if (m_iter == std::end(*m_range))
        {
            return {};
        }
        return *m_iter++;
    }
};

}  // namespace detail

template <class T>
struct sequence : detail::inspect_mixin<T>,
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
                  detail::for_each_indexed_mixin<T>,
                  detail::associate_mixin<T>
{
    using iterator = detail::sequence_iterator<T>;
    using next_function_type = typename iterator::next_function_type;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer = typename iterator::pointer;
    using difference_type = typename iterator::difference_type;

    next_function_type m_next_fn;

    explicit sequence(next_function_type next_fn) : m_next_fn(std::move(next_fn))
    {
    }

    template <class U, require<std::is_constructible_v<reference, U>> = 0>
    sequence(const sequence<U>& other) : sequence(detail::cast_sequence<reference, U>{ other.get_next_function() })
    {
    }

    template <class U, require<std::is_constructible_v<reference, U>> = 0>
    sequence(sequence<U>&& other) : sequence(detail::cast_sequence<reference, U>{ std::move(other).get_next_function() })
    {
    }

    template <class Iter, require<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    sequence(Iter b, Iter e) : sequence(detail::view_sequence<Iter, reference>{ b, e })
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        require<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    sequence(Range&& range) : sequence(std::begin(range), std::end(range))
    {
    }

    template <
        class Range,
        class Iter = iterator_t<Range>,
        require<std::is_constructible_v<reference, iter_reference_t<Iter>>> = 0>
    sequence(Range range, int)
        : sequence(detail::owning_sequence<Range, Iter, reference>{ std::make_shared<Range>(std::move(range)) })
    {
    }

    sequence() : sequence(detail::empty_sequence<T>{})
    {
    }

    template <class Container, require<std::is_constructible_v<Container, iterator, iterator>> = 0>
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

    auto front() const& -> reference
    {
        return maybe_front().value();
    }

    auto maybe_at(difference_type n) const -> maybe<reference>
    {
        return this->drop(n).maybe_front();
    }

    auto at(difference_type n) const -> reference
    {
        return maybe_at(n).value();
    }

    bool empty() const
    {
        return begin() == end();
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

    auto slice(slice_t info) const -> sequence<T>
    {
        if (info.begin && info.end)
        {
            return this->drop(*info.begin).take(std::max(*info.end - *info.begin, difference_type{ 0 }));
        }
        if (info.begin)
        {
            return this->drop(*info.begin);
        }
        if (info.end)
        {
            return this->take(*info.end);
        }
        return *this;
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
    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range&& range) const -> sequence<Out>
    {
        return sequence<Out>{ view_sequence<iterator_t<Range>, Out>{ std::begin(range), std::end(range) } };
    }

    template <class Iter, class Out = iter_reference_t<Iter>>
    auto operator()(Iter b, Iter e) const -> sequence<Out>
    {
        return sequence<Out>{ view_sequence<Iter, Out>{ b, e } };
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
    template <class Range, class Out = range_reference_t<Range>>
    auto operator()(Range range) const -> sequence<Out>
    {
        return sequence<Out>{ owning_sequence<Range, iterator_t<Range>, Out>{ std::make_shared<Range>(std::move(range)) } };
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

/*
         _                      _____   ____
   ___  | |__     __ _   _ __  |___ /  |___ \
  / __| | '_ \   / _` | | '__|   |_ \    __) |
 | (__  | | | | | (_| | | |     ___) |  / __/
  \___| |_| |_|  \__,_| |_|    |____/  |_____|
*/

struct char32
{
    char32_t m_data;

    char32() = default;

    char32(char32_t data) : m_data(data)
    {
    }

    char32(std::string_view txt) : char32()
    {
        const auto res = read(txt);
        assert_that<std::runtime_error>(res.has_value(), "char32: invalid input");
        assert_that<std::runtime_error>(res->second.empty(), "char32: invalid input - exactly one glyph expected");
        m_data = res->first.m_data;
    }

    char32(const char ch) : char32(std::string_view(&ch, 1))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const char32& item)
    {
        std::array<char, 4> data;
        const span<char> v = encode(data, item.m_data);
        std::copy(v.begin(), v.end(), std::ostream_iterator<char>{ os });
        return os;
    }

    static auto split(std::string_view text) -> sequence<char32>
    {
        return sequence<char32>{ [=]() mutable -> iteration_result_t<char32>
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

    static auto split(std::u32string_view text) -> sequence<char32>
    {
        return seq::view(text).transform([](char32_t ch) { return char32{ ch }; });
    }

private:
    static auto read(std::string_view txt) -> maybe<std::pair<char32, std::string_view>>
    {
        const auto [rc, c32] = decode(txt);
        if (rc == std::size_t(0) || rc == std::size_t(-1) || rc == std::size_t(-2))
        {
            return {};
        }
        txt.remove_prefix(rc);
        return std::pair{ char32{ c32 }, txt };
    }

    static auto decode(std::string_view txt) -> std::pair<std::size_t, char32_t>
    {
        std::setlocale(LC_ALL, "en_US.utf8");
        std::mbstate_t state{};
        char32_t c32 = {};
        std::size_t rc = std::mbrtoc32(&c32, txt.begin(), txt.size(), &state);
        assert_that<std::runtime_error>(rc != std::size_t(-3), "mbrtoc32: error in conversion from char[] to char32_t");
        return { rc, c32 };
    }

    static auto encode(std::array<char, 4>& out, char32_t value) -> span<char>
    {
        std::mbstate_t state{};
        const std::uint8_t size = std::c32rtomb(out.data(), value, &state);
        assert_that<std::runtime_error>(size != std::size_t(-1), "c32rtomb: error in conversion from char32_t to char[]");
        return span<char>{ out.data(), out.data() + size };
    }

    friend constexpr bool operator==(const char32& lhs, const char32& rhs)
    {
        return lhs.m_data == rhs.m_data;
    }

    friend constexpr bool operator!=(const char32& lhs, const char32& rhs)
    {
        return !(lhs == rhs);
    }

    friend constexpr bool operator<(const char32& lhs, const char32& rhs)
    {
        return lhs.m_data < rhs.m_data;
    }

    friend constexpr bool operator>(const char32& lhs, const char32& rhs)
    {
        return rhs < lhs;
    }

    friend constexpr bool operator<=(const char32& lhs, const char32& rhs)
    {
        return !(lhs > rhs);
    }

    friend constexpr bool operator>=(const char32& lhs, const char32& rhs)
    {
        return !(lhs < rhs);
    }
};

/*
                            _   _                  _
  _ __    _ __    ___    __| | (_)   ___    __ _  | |_    ___   ___
 | '_ \  | '__|  / _ \  / _` | | |  / __|  / _` | | __|  / _ \ / __|
 | |_) | | |    |  __/ | (_| | | | | (__  | (_| | | |_  |  __/ \__ \
 | .__/  |_|     \___|  \__,_| |_|  \___|  \__,_|  \__|  \___| |___/
 |_|
*/

namespace detail
{

struct unwrap_fn
{
    template <class T>
    constexpr auto operator()(T& item) const -> T&
    {
        return item;
    }

    template <class T>
    constexpr auto operator()(std::reference_wrapper<T> item) const -> T&
    {
        return item;
    }
};

static constexpr inline auto unwrap = unwrap_fn{};

ZX_DEFINE_IS_DETECTED_2(is_equality_comparable, std::declval<T0>() == std::declval<T1>());

template <class Pred, class T>
constexpr bool invoke_pred(Pred&& pred, T&& item)
{
    if constexpr (std::is_invocable_v<Pred, T>)
    {
        return std::invoke(std::forward<Pred>(pred), std::forward<T>(item));
    }
    else if constexpr (is_equality_comparable_v<T, Pred>)
    {
        return pred == item;
    }
    else
    {
        static_assert(always_false<T>::value, "type must be either equality comparable or invocable");
    }
}

struct make_predicate_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;
        std::string name;

        template <class U>
        constexpr bool operator()(U&& item) const
        {
            return invoke_pred(pred, std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, item.name);
        }
    };

    template <class Pred>
    auto operator()(Pred pred, std::string name) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred), std::move(name) };
    }
};

struct all_tag
{
};
struct any_tag
{
};

template <class Tag, class Name>
struct compound_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        constexpr bool operator()(U&& item) const
        {
            if constexpr (std::is_same_v<Tag, any_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... || invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
            else if constexpr (std::is_same_v<Tag, all_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... && invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};

            format_to(os, "(", name);
            std::apply([&](const auto&... preds) { ((format_to(os, ' ', preds)), ...); }, item.m_preds);
            return format_to(os, ")");
        }
    };

    template <class Pipe>
    auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    auto to_tuple(impl<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_preds;
    }

    template <class... Pipes>
    auto from_tuple(std::tuple<Pipes...> tuple) const -> impl<Pipes...>
    {
        return impl<Pipes...>{ std::move(tuple) };
    }

    template <class... Pipes>
    auto operator()(Pipes... pipes) const -> decltype(from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...));
    }
};

struct negate_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return !invoke_pred(m_pred, std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(not ", item.m_pred, ")");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct is_some_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item) && invoke_pred(m_pred, *std::forward<U>(item));
        }

        bool operator()(nullptr_t) const
        {
            return false;
        }

        bool operator()(std::nullopt_t) const
        {
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(is_some ", item.m_pred, ")");
        }
    };

    struct void_impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item);
        }

        bool operator()(nullptr_t) const
        {
            return false;
        }

        bool operator()(std::nullopt_t) const
        {
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const void_impl& item)
        {
            return format_to(os, "(is_some)");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }

    auto operator()() const -> void_impl
    {
        return void_impl{};
    }
};

struct is_none_fn
{
    struct impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return !static_cast<bool>(item);
        }

        bool operator()(nullptr_t) const
        {
            return true;
        }

        bool operator()(std::nullopt_t) const
        {
            return true;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(is_none)");
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

template <class Op, class Name>
struct compare_fn
{
    template <class T>
    struct impl
    {
        T m_value;

        template <class U>
        bool operator()(U&& item) const
        {
            static const auto op = Op{};
            return op(std::forward<U>(item), m_value);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return format_to(os, "(", name, ' ', item.m_value, ")");
        }
    };

    template <class T>
    auto operator()(T&& value) const -> impl<std::decay_t<T>>
    {
        return impl<std::decay_t<T>>{ std::forward<T>(value) };
    }
};

struct size_is_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(m_pred, std::distance(std::begin(item), std::end(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(size_is ", item.m_pred, ")");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct is_empty_fn
{
    struct impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return std::begin(item) == std::end(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(is_empty)");
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

struct each_item_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::all_of(
                std::begin(item),
                std::end(item),
                [&](auto&& v) { return invoke_pred(m_pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(each_item ", item.m_pred, ")");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct contains_item_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::any_of(
                std::begin(item),
                std::end(item),
                [&](auto&& v) { return invoke_pred(m_pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(contains_item ", item.m_pred, ")");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct no_item_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::none_of(
                std::begin(item),
                std::end(item),
                [&](auto&& v) { return invoke_pred(m_pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(no_item ", item.m_pred, ")");
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct items_are_fn
{
    template <std::size_t N = 0, class... Preds, class Iter>
    static bool call(const std::tuple<Preds...>& preds, Iter begin, Iter end)
    {
        if constexpr (N == sizeof...(Preds))
        {
            return begin == end;
        }
        else
        {
            return begin != end && invoke_pred(std::get<N>(preds), *begin) && call<N + 1>(preds, std::next(begin), end);
        }
    }
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            return call(m_preds, std::begin(item), std::end(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            format_to(os, "(items_are");
            std::apply([&](const auto&... preds) { ((format_to(os, ' ', preds)), ...); }, item.m_preds);
            return format_to(os, ")");
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct items_are_array_fn
{
    template <class PIter, class Iter>
    static bool call(PIter p_b, PIter p_e, Iter begin, Iter end)
    {
        return std::equal(p_b, p_e, begin, end, [](auto&& p, auto&& it) { return invoke_pred(p, it); });
    }
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            return call(std::begin(unwrap(m_range)), std::end(unwrap(m_range)), std::begin(item), std::end(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(", "items_are_array ", item.m_range, ")");
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct starts_with_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_fn ::call(m_preds, b, std::next(b, preds_count));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            format_to(os, "(starts_with_items");
            std::apply([&](const auto&... preds) { ((format_to(os, ' ', preds)), ...); }, item.m_preds);
            return format_to(os, ")");
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct starts_with_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_array_fn::call(p_b, p_e, b, std::next(b, preds_count));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(", "starts_with_array ", item.m_range, ")");
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct ends_with_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_fn::call(m_preds, std::next(b, size - preds_count), e);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            format_to(os, "(ends_with_items");
            std::apply([&](const auto&... preds) { ((format_to(os, ' ', preds)), ...); }, item.m_preds);
            return format_to(os, ")");
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct ends_with_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_array_fn::call(p_b, p_e, std::next(b, size - preds_count), e);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(", "ends_with_array ", item.m_range, ")");
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct contains_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            if (size < preds_count)
            {
                return false;
            }
            for (std::size_t i = 0; i < size - preds_count + 1; ++i)
            {
                if (items_are_fn::call(m_preds, std::next(b, i), std::next(b, i + preds_count)))
                {
                    return true;
                }
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            format_to(os, "(contains_items");
            std::apply([&](const auto&... preds) { ((format_to(os, ' ', preds)), ...); }, item.m_preds);
            return format_to(os, ")");
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct contains_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            if (size < preds_count)
            {
                return false;
            }
            for (std::size_t i = 0; i < size - preds_count + 1; ++i)
            {
                if (items_are_array_fn::call(p_b, p_e, std::next(b, i), std::next(b, i + preds_count)))
                {
                    return true;
                }
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return format_to(os, "(", "contains_array ", item.m_range, ")");
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

template <class Name>
struct result_of_fn
{
    template <class Func, class Pred>
    struct impl
    {
        Func m_func;
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(m_pred, std::invoke(m_func, std::forward<U>(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return format_to(os, "(", name, ' ', item.m_func, ' ', item.m_pred, ")");
        }
    };

    template <class Func, class Pred>
    auto operator()(Func&& func, Pred&& pred) const -> impl<std::decay_t<Func>, std::decay_t<Pred>>
    {
        return { std::forward<Func>(func), std::forward<Pred>(pred) };
    }
};
}  // namespace detail

template <class T>
struct predicate : std::function<bool(in_t<T>)>
{
    using base_t = std::function<bool(in_t<T>)>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const predicate& item)
    {
        return os << "predicate<" << type<T>{} << ">";
    }
};

struct assertion_error : std::runtime_error
{
    explicit assertion_error(std::string msg) : std::runtime_error(std::move(msg))
    {
    }
};

template <char... Ch>
struct str_t
{
    static std::string_view value()
    {
        static const auto instance = std::string{ Ch... };
        return instance;
    }

    operator std::string_view() const
    {
        return value();
    }

    friend std::ostream& operator<<(std::ostream& os, const str_t)
    {
        return os << value();
    }
};

static constexpr inline auto make_predicate = detail::make_predicate_fn{};

static constexpr inline auto any = detail::compound_fn<detail::any_tag, str_t<'a', 'n', 'y'>>{};
static constexpr inline auto all = detail::compound_fn<detail::all_tag, str_t<'a', 'l', 'l'>>{};
static constexpr inline auto negate = detail::negate_fn{};

static constexpr inline auto eq = detail::compare_fn<std::equal_to<>, str_t<'e', 'q'>>{};
static constexpr inline auto ne = detail::compare_fn<std::not_equal_to<>, str_t<'n', 'e'>>{};
static constexpr inline auto lt = detail::compare_fn<std::less<>, str_t<'l', 't'>>{};
static constexpr inline auto gt = detail::compare_fn<std::greater<>, str_t<'g', 't'>>{};
static constexpr inline auto le = detail::compare_fn<std::less_equal<>, str_t<'l', 'e'>>{};
static constexpr inline auto ge = detail::compare_fn<std::greater_equal<>, str_t<'g', 'e'>>{};

static constexpr inline auto each_item = detail::each_item_fn{};
static constexpr inline auto contains_item = detail::contains_item_fn{};
static constexpr inline auto no_item = detail::no_item_fn{};
static constexpr inline auto size_is = detail::size_is_fn{};
static constexpr inline auto is_empty = detail::is_empty_fn{};

static constexpr inline auto items_are = detail::items_are_fn{};
static constexpr inline auto items_are_array = detail::items_are_array_fn{};
static constexpr inline auto starts_with_items = detail::starts_with_items_fn{};
static constexpr inline auto starts_with_array = detail::starts_with_array_fn{};
static constexpr inline auto ends_with_items = detail::ends_with_items_fn{};
static constexpr inline auto ends_with_array = detail::ends_with_array_fn{};
static constexpr inline auto contains_items = detail::contains_items_fn{};
static constexpr inline auto contains_array = detail::contains_array_fn{};

static constexpr inline auto result_of = detail::result_of_fn<str_t<'r', 'e', 's', 'u', 'l', 't', '_', 'o', 'f'>>{};
static constexpr inline auto field = detail::result_of_fn<str_t<'f', 'i', 'e', 'l', 'd'>>{};
static constexpr inline auto property = detail::result_of_fn<str_t<'p', 'r', 'o', 'p', 'e', 'r', 't', 'y'>>{};

static constexpr inline auto is_some = detail::is_some_fn{};
static constexpr inline auto is_none = detail::is_none_fn{};

}  // namespace zx

#define ZX_SOURCE_LOCATION ::zx::source_location(__FILE__, __LINE__, __PRETTY_FUNCTION__)

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
