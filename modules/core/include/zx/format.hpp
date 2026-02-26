#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <tuple>
#include <zx/type_traits.hpp>

namespace zx
{

struct ostream_writer_t : public std::function<void(std::ostream&)>
{
    using base_type = std::function<void(std::ostream&)>;
    using base_type::base_type;

    friend std::ostream& operator<<(std::ostream& os, const ostream_writer_t& item)
    {
        item(os);
        return os;
    }
};

template <class T, class = void>
struct formatter;

namespace detail
{

template <class T, class = void>
struct has_formatter : std::false_type
{
};

template <class T>
struct has_formatter<
    T,
    std::void_t<decltype(std::declval<formatter<T>>().format(std::declval<std::ostream&>(), std::declval<const T&>()))>>
    : std::true_type
{
};

template <class T, class = void>
struct has_ostream_operator : std::false_type
{
};

template <class T>
struct has_ostream_operator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type
{
};

static constexpr inline struct format_to_fn
{
    template <class T>
    static void do_format(std::ostream& os, const T& item)
    {
        if constexpr (has_formatter<T>::value)
        {
            formatter<T>{}.format(os, item);
        }
        else if constexpr (has_ostream_operator<T>::value)
        {
            os << item;
        }
        else
        {
            static_assert(always_false<T>::value, "No formatter or ostream operator available for type T");
        }
    }

    template <class... Args>
    auto operator()(std::ostream& os, Args&&... args) const -> std::ostream&
    {
        (do_format(os, std::forward<Args>(args)), ...);
        return os;
    }
} format_to = {};

static constexpr inline struct format_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::string
    {
        std::stringstream ss;
        format_to(ss, std::forward<Args>(args)...);
        return std::move(ss).str();
    }
} format = {};

static constexpr inline struct print_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::ostream&
    {
        return format_to(std::cout, std::forward<Args>(args)...);
    }
} print = {};

static constexpr inline struct println_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::ostream&
    {
        return print(std::forward<Args>(args)...) << std::endl;
    }
} println = {};

static constexpr inline struct delimit_fn
{
    template <class Iter>
    auto operator()(Iter begin, Iter end, std::string_view separator) const -> ostream_writer_t
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
    auto operator()(Range&& range, std::string_view separator) const -> ostream_writer_t
    {
        return (*this)(std::begin(range), std::end(range), separator);
    }

    template <class T>
    auto operator()(std::initializer_list<T> range, std::string_view separator) const -> ostream_writer_t
    {
        return (*this)(std::begin(range), std::end(range), separator);
    }

    auto operator()(const char* str, std::string_view separator) const -> ostream_writer_t
    {
        return (*this)(std::string_view(str), separator);
    }
} delimit = {};

}  // namespace detail

using detail::delimit;
using detail::format;
using detail::format_to;
using detail::print;
using detail::println;
static constexpr inline auto str = format;

template <>
struct formatter<bool>
{
    void format(std::ostream& os, const bool item) const { os << (item ? "true" : "false"); }
};

struct tuple_formatter
{
    template <class Tuple>
    void format(std::ostream& os, const Tuple& item) const
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

struct range_formatter
{
    template <class Range>
    void format(std::ostream& os, const Range& item) const
    {
        os << "[" << delimit(item, ", ") << "]";
    }
};

template <class... Args>
struct formatter<std::tuple<Args...>> : tuple_formatter
{
};

template <class F, class S>
struct formatter<std::pair<F, S>> : tuple_formatter
{
};

template <class T>
struct formatter<std::optional<T>>
{
    void format(std::ostream& os, const std::optional<T>& item) const
    {
        if (item)
        {
            format_to(os, *item);
        }
        else
        {
            os << "<< none >>";
        }
    }
};

}  // namespace zx
