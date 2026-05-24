#pragma once

#include <algorithm>
#include <zx/maybe.hpp>
#include <zx/nested_text.hpp>

namespace zx
{

namespace predicates
{

template <class T>
nested_text::node_t encode_value(const T& item)
{
    if constexpr (nested_text::has_encode<T>::value)
    {
        return nested_text::encode(item);
    }
    else
    {
        return nested_text::string_t{ type_name<T>() };
    }
}

struct validation_error_t
{
    nested_text::node_t expected;
    nested_text::node_t actual;
    nested_text::node_t reason;

    nested_text::node_t encode() const
    {
        return nested_text::map_t{
            { "expected", expected },
            { "actual", actual },
            { "reason", reason },
        };
    }

    friend std::ostream& operator<<(std::ostream& os, const validation_error_t& item) { return os << item.encode(); }
};

struct validation_result_t
{
    maybe_t<validation_error_t> m_error;

    static validation_result_t success() { return validation_result_t{ none }; }

    static validation_result_t failure(validation_error_t error) { return validation_result_t{ std::move(error) }; }

    bool is_success() const { return !m_error.has_value(); }

    const validation_error_t& error() const
    {
        if (is_success())
        {
            throw std::logic_error{ "validation_result_t does not contain an error" };
        }
        return *m_error;
    }
};

template <class Impl, class T>
using has_test_impl = decltype(std::declval<const Impl&>().test(std::declval<const T&>()));

template <class Impl, class T>
using has_validate_impl = decltype(std::declval<const Impl&>().validate(std::declval<T&>()));

template <class Impl, class T>
struct has_test : is_detected<has_test_impl, Impl, T>
{
};

template <class Impl, class T>
struct has_validate : is_detected<has_validate_impl, Impl, T>
{
};

template <class L, class R>
using is_equality_comparable_impl = decltype(std::declval<const L&>() == std::declval<const R&>());

template <class L, class R>
struct is_equality_comparable : is_detected<is_equality_comparable_impl, L, R>
{
};

template <class Impl>
using has_format_impl = decltype(std::declval<const Impl&>().format());

template <class Impl>
struct has_format : is_detected<has_format_impl, Impl>
{
};

template <class Impl>
struct predicate_interface_t
{
    Impl m_impl;

    template <class T>
    bool test(const T& value) const
    {
        if constexpr (has_test<Impl, T>::value)
        {
            return m_impl.test(value);
        }
        else if constexpr (is_equality_comparable<Impl, T>::value)
        {
            return value == m_impl;
        }
        else
        {
            static_assert(
                always_false<Impl, T>::value,
                "predicate implementation needs to provide a test() method or be directly comparable to the tested value");
        }
    }

    template <class T>
    bool operator()(const T& value) const
    {
        return test(value);
    }

    template <class T>
    validation_result_t validate(const T& value) const
    {
        if constexpr (has_validate<Impl, T>::value)
        {
            return m_impl.validate(value);
        }
        else if constexpr (has_test<Impl, T>::value)
        {
            return m_impl.test(value) ? validation_result_t::success()
                                      : validation_result_t::failure(validation_error_t{
                                          format(), encode_value(value), nested_text::node_t{ "predicate test failed" } });
        }
        else if constexpr (is_equality_comparable<Impl, T>::value)
        {
            return value == m_impl ? validation_result_t::success()
                                   : validation_result_t::failure(validation_error_t{
                                       format(), encode_value(value), nested_text::node_t{ "values are not equal" } });
        }
        else
        {
            static_assert(
                always_false<Impl, T>::value,
                "predicate implementation needs to provide a validate() or test() method or be directly comparable to the "
                "tested value");
        }
    }

    nested_text::node_t format() const
    {
        if constexpr (has_format<Impl>::value)
        {
            return m_impl.format();
        }
        else
        {
            return encode_value(m_impl);
        }
    }
};

template <class Impl>
predicate_interface_t(Impl&&) -> predicate_interface_t<std::decay_t<Impl>>;

template <class T>
constexpr auto to_predicate_interface(T&& value) -> predicate_interface_t<std::decay_t<T>>
{
    return predicate_interface_t<std::decay_t<T>>{ std::forward<T>(value) };
}

template <class T>
constexpr auto to_predicate_interface(predicate_interface_t<T> pred) -> predicate_interface_t<T>
{
    return pred;
}

namespace detail
{

struct eq_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value == m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "value is not equal to expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "eq", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct ne_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value != m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "value is equal to expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "ne", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct gt_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value > m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "value is not greater than expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "gt", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct lt_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value < m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "value is not less than expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "lt", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct le_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value <= m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "value is not less than or equal to expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "le", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct ge_fn
{
    template <class T>
    struct impl_t
    {
        T m_expected;

        template <class U>
        bool test(const U& value) const
        {
            return value >= m_expected;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(),
                      encode_value(value),
                      nested_text::node_t{ "value is not greater than or equal to expected" } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "ge", encode_value(m_expected) }; }
    };

    template <class T>
    constexpr auto operator()(T expected) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(expected) } };
    }
};

struct divisible_by_fn
{
    template <class T>
    struct impl_t
    {
        T m_divisor;

        template <class U>
        bool test(const U& value) const
        {
            return value % m_divisor == 0;
        }

        template <class U>
        validation_result_t validate(const U& value) const
        {
            if (test(value))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ str("value is not divisible by ", m_divisor) } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "divisible_by", str(m_divisor) }; }
    };

    template <class T>
    constexpr auto operator()(T divisor) const
    {
        return predicate_interface_t{ impl_t<T>{ std::move(divisor) } };
    }
};

struct any_fn
{
    template <class Predicates>
    struct impl_t
    {
        Predicates m_predicates;

        impl_t(Predicates predicates) : m_predicates(std::move(predicates)) { }

        template <class T>
        bool test(const T& value) const
        {
            return std::apply([&](const auto&... pred) { return (... || pred.test(value)); }, m_predicates);
        }

        template <class T>
        validation_result_t validate(const T& value) const
        {
            std::vector<validation_result_t> results = {};
            std::apply([&](const auto&... pred) { (..., results.push_back(pred.validate(value))); }, m_predicates);
            if (std::any_of(
                    results.begin(), results.end(), [](const validation_result_t& result) { return result.is_success(); }))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    { format(),
                      encode_value(value),
                      nested_text::list_t{ nested_text::node_t{ "none of the predicates matched" } } });
            }
        }

        nested_text::node_t format() const
        {
            nested_text::list_t result = {};
            std::apply([&](const auto&... pred) { (result.push_back(pred.format()), ...); }, m_predicates);
            return nested_text::list_t{ "any", std::move(result) };
        }
    };

    template <class... Predicates>
    constexpr auto operator()(Predicates&&... predicates) const
    {
        return predicate_interface_t{ impl_t{
            std::tuple{ to_predicate_interface(std::forward<Predicates>(predicates))... } } };
    }
};

struct all_fn
{
    template <class Predicates>
    struct impl_t
    {
        Predicates m_predicates;

        impl_t(Predicates predicates) : m_predicates(std::move(predicates)) { }

        template <class T>
        bool test(const T& value) const
        {
            return std::apply([&](const auto&... pred) { return (... && pred.test(value)); }, m_predicates);
        }

        template <class T>
        validation_result_t validate(const T& value) const
        {
            std::vector<validation_result_t> results = {};
            std::apply([&](const auto&... pred) { (..., results.push_back(pred.validate(value))); }, m_predicates);
            if (std::all_of(
                    results.begin(), results.end(), [](const validation_result_t& result) { return result.is_success(); }))
            {
                return validation_result_t::success();
            }
            else
            {
                nested_text::list_t errors = {};
                for (const validation_result_t& result : results)
                {
                    if (!result.is_success())
                    {
                        errors.push_back(nested_text::map_t{ { "predicate", result.error().expected },
                                                             { "reason", result.error().reason } });
                    }
                }
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::map_t{ { "failing_predicates", std::move(errors) } } });
            }
        }

        nested_text::node_t format() const
        {
            nested_text::list_t result = {};
            std::apply([&](const auto&... pred) { (result.push_back(pred.format()), ...); }, m_predicates);
            return nested_text::list_t{ "all", std::move(result) };
        }
    };

    template <class... Predicates>
    constexpr auto operator()(Predicates&&... predicates) const
    {
        return predicate_interface_t{ impl_t{
            std::tuple{ to_predicate_interface(std::forward<Predicates>(predicates))... } } };
    }
};

struct is_not_fn
{
    template <class Predicate>
    struct impl_t
    {
        Predicate m_predicate;

        impl_t(Predicate predicate) : m_predicate(std::move(predicate)) { }

        template <class T>
        bool test(const T& value) const
        {
            return !m_predicate.test(value);
        }

        template <class T>
        validation_result_t validate(const T& value) const
        {
            const auto res = m_predicate.validate(value);
            if (res.is_success())
            {
                return validation_result_t::failure(
                    { format(), encode_value(value), nested_text::node_t{ "predicate matched but was expected not to" } });
            }
            return validation_result_t::success();
        }

        nested_text::node_t format() const { return nested_text::list_t{ "is_not", m_predicate.format() }; }
    };

    template <class Predicate>
    constexpr auto operator()(Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

struct items_all_fn
{
    template <class Predicate>
    struct impl_t
    {
        Predicate m_predicate;

        impl_t(Predicate predicate) : m_predicate(std::move(predicate)) { }

        template <class Range>
        bool test(const Range& range) const
        {
            return std::all_of(
                std::begin(range), std::end(range), [&](const auto& element) { return m_predicate.test(element); });
        }

        template <class Range>
        validation_result_t validate(const Range& range) const
        {
            std::size_t index = 0;
            const auto begin = std::begin(range);
            const auto end = std::end(range);
            nested_text::list_t errors = {};
            for (auto it = begin; it != end; ++it, ++index)
            {
                const auto res = m_predicate.validate(*it);
                if (!res.is_success())
                {
                    errors.push_back(nested_text::map_t{
                        { "index", str(index) },
                        { "value", res.error().actual },
                        { "reason", res.error().reason },
                    });
                }
            }
            return errors.empty()
                       ? validation_result_t::success()
                       : validation_result_t::failure(validation_error_t{
                           format(), encode_value(range), nested_text::map_t{ { "failing_items", std::move(errors) } } });
        }

        nested_text::node_t format() const { return nested_text::list_t{ "items_all", m_predicate.format() }; }
    };

    template <class Predicate>
    constexpr auto operator()(Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

struct items_any_fn
{
    template <class Predicate>
    struct impl_t
    {
        Predicate m_predicate;

        impl_t(Predicate predicate) : m_predicate(std::move(predicate)) { }

        template <class Range>
        bool test(const Range& range) const
        {
            return std::any_of(
                std::begin(range), std::end(range), [&](const auto& element) { return m_predicate.test(element); });
        }

        template <class Range>
        validation_result_t validate(const Range& range) const
        {
            const auto begin = std::begin(range);
            const auto end = std::end(range);
            for (auto it = begin; it != end; ++it)
            {
                if (m_predicate.test(*it))
                {
                    return validation_result_t::success();
                }
            }
            return validation_result_t::failure(validation_error_t{
                format(),
                encode_value(range),
                nested_text::node_t{ "no items in the range matched the predicate" },
            });
        }

        nested_text::node_t format() const { return nested_text::list_t{ "items_any", m_predicate.format() }; }
    };

    template <class Predicate>
    constexpr auto operator()(Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

struct items_none_fn
{
    template <class Predicate>
    struct impl_t
    {
        Predicate m_predicate;

        impl_t(Predicate predicate) : m_predicate(std::move(predicate)) { }

        template <class Range>
        bool test(const Range& range) const
        {
            return !std::any_of(
                std::begin(range), std::end(range), [&](const auto& element) { return m_predicate.test(element); });
        }

        template <class Range>
        validation_result_t validate(const Range& range) const
        {
            std::size_t index = 0;
            const auto begin = std::begin(range);
            const auto end = std::end(range);
            nested_text::list_t results = {};
            for (auto it = begin; it != end; ++it, ++index)
            {
                const auto res = m_predicate.validate(*it);
                if (res.is_success())
                {
                    results.push_back(nested_text::map_t{
                        { "index", str(index) },
                        { "value", encode_value(*it) },
                    });
                }
            }
            return results.empty()
                       ? validation_result_t::success()
                       : validation_result_t::failure(validation_error_t{
                           format(), encode_value(range), nested_text::map_t{ { "matching_items", std::move(results) } } });
        }

        nested_text::node_t format() const { return nested_text::list_t{ "items_none", m_predicate.format() }; }
    };

    template <class Predicate>
    constexpr auto operator()(Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

struct items_count_fn
{
    template <class Predicate>
    struct impl_t
    {
        Predicate m_predicate;

        impl_t(Predicate predicate) : m_predicate(std::move(predicate)) { }

        template <class Range>
        bool test(const Range& range) const
        {
            return m_predicate.test(std::distance(std::begin(range), std::end(range)));
        }

        template <class Range>
        validation_result_t validate(const Range& range) const
        {
            const auto count = std::distance(std::begin(range), std::end(range));
            const auto res = m_predicate.validate(count);
            if (res.is_success())
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(validation_error_t{
                    format(),
                    encode_value(range),
                    nested_text::list_t{ "number of items in the range does not match the expected count",
                                         nested_text::map_t{ { "expected_count", m_predicate.format() },
                                                             { "actual_count", encode_value(count) } } } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "items_count", m_predicate.format() }; }
    };

    template <class Predicate>
    constexpr auto operator()(Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

struct items_are_fn
{
    template <class... Predicates>
    struct impl_t
    {
        std::tuple<Predicates...> m_predicates;

        impl_t(Predicates... predicates) : m_predicates(std::move(predicates)...) { }

        template <class Range>
        bool test(const Range& range) const
        {
            auto begin = std::begin(range);
            const auto end = std::end(range);
            return std::apply(
                [&](const auto&... pred)
                { return (... && (begin != end && pred.test(*begin++) && true)) && (begin == end); },
                m_predicates);
        }

        template <class Range>
        validation_result_t validate(const Range& range) const
        {
            auto it = std::begin(range);
            const auto end = std::end(range);
            const auto actual_count = static_cast<std::size_t>(std::distance(it, end));
            constexpr auto expected_count = sizeof...(Predicates);
            std::size_t index = 0;
            nested_text::list_t errors = {};
            std::apply(
                [&](const auto&... pred)
                {
                    (...,
                     std::invoke(
                         [&]()
                         {
                             const auto i = index++;
                             if (it != end)
                             {
                                 const auto res = pred.validate(*it);
                                 ++it;
                                 if (!res.is_success())
                                 {
                                     errors.push_back(nested_text::map_t{
                                         { "index", str(i) },
                                         { "value", res.error().actual },
                                         { "reason", res.error().reason },
                                     });
                                 }
                             }
                         }));
                },
                m_predicates);
            if (errors.empty() && actual_count == expected_count)
            {
                return validation_result_t::success();
            }
            auto reason = actual_count != expected_count
                ? nested_text::map_t{
                    { "expected_count", nested_text::node_t{ str(expected_count) } },
                    { "actual_count", nested_text::node_t{ str(actual_count) } },
                    { "failing_items", std::move(errors) },
                }
                : nested_text::map_t{ { "failing_items", std::move(errors) } };
            return validation_result_t::failure(validation_error_t{ format(), encode_value(range), std::move(reason) });
        }

        nested_text::node_t format() const
        {
            nested_text::list_t result;
            std::apply([&](const auto&... pred) { (result.push_back(pred.format()), ...); }, m_predicates);
            return nested_text::list_t{ "items_are", std::move(result) };
        }
    };

    template <class... Predicates>
    constexpr auto operator()(Predicates&&... predicates) const
    {
        return predicate_interface_t{ impl_t{ to_predicate_interface(std::forward<Predicates>(predicates))... } };
    }
};

struct result_of_fn
{
    template <class Func, class Predicate>
    struct impl_t
    {
        std::string m_name;
        Func m_func;
        Predicate m_predicate;

        impl_t(std::string name, Func func, Predicate predicate)
            : m_name(std::move(name))
            , m_func(std::move(func))
            , m_predicate(std::move(predicate))
        {
        }

        template <class T>
        bool test(const T& value) const
        {
            return m_predicate.test(std::invoke(m_func, value));
        }

        template <class T>
        validation_result_t validate(const T& value) const
        {
            const auto& result = std::invoke(m_func, value);
            const auto res = m_predicate.validate(result);
            if (res.is_success())
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(validation_error_t{
                    format(),
                    encode_value(value),
                    nested_text::list_t{ "result of the function did not satisfy the predicate",
                                         nested_text::map_t{ { "function_name", m_name },
                                                             { "function_result", encode_value(result) },
                                                             { "predicate_error", res.error().encode() } } } });
            }
        }

        nested_text::node_t format() const { return nested_text::list_t{ "result_of", m_name, m_predicate.format() }; }
    };

    template <class Func, class Predicate>
    auto operator()(std::string name, Func&& func, Predicate&& predicate) const
    {
        return predicate_interface_t{ impl_t{
            std::move(name), std::forward<Func>(func), to_predicate_interface(std::forward<Predicate>(predicate)) } };
    }
};

}  // namespace detail

static constexpr inline auto eq = detail::eq_fn{};
static constexpr inline auto ne = detail::ne_fn{};
static constexpr inline auto gt = detail::gt_fn{};
static constexpr inline auto lt = detail::lt_fn{};
static constexpr inline auto le = detail::le_fn{};
static constexpr inline auto ge = detail::ge_fn{};

static constexpr inline auto divisible_by = detail::divisible_by_fn{};

static constexpr inline auto any = detail::any_fn{};
static constexpr inline auto all = detail::all_fn{};
static constexpr inline auto is_not = detail::is_not_fn{};

static constexpr inline auto items_all = detail::items_all_fn{};
static constexpr inline auto items_any = detail::items_any_fn{};
static constexpr inline auto items_none = detail::items_none_fn{};
static constexpr inline auto items_count = detail::items_count_fn{};
static constexpr inline auto items_are = detail::items_are_fn{};

static constexpr inline auto result_of = detail::result_of_fn{};
static constexpr inline auto field = result_of;
static constexpr inline auto property = result_of;

}  // namespace predicates
}  // namespace zx
