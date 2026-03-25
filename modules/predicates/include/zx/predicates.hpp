#pragma once

#include <zx/nested_text.hpp>
#include <zx/type_name.hpp>

namespace zx
{
namespace predicates
{

struct value_info_t
{
    nested_text::value_t value;
    nested_text::string_t type_name;
};

struct validation_result_t
{
    struct failure_t
    {
        nested_text::value_t failing_predicate;
        value_info_t actual;
        nested_text::value_t failure_message;

        nested_text::value_t format() const
        {
            return nested_text::map_t{ { "actual",
                                         nested_text::map_t{ { "type", actual.type_name }, { "value", actual.value } } },
                                       { "failing_predicate", failing_predicate },
                                       { "failure_message", failure_message } };
        }
    };

    bool m_success;
    failure_t m_failure;

    static validation_result_t success() { return { true, { {}, {}, {} } }; }

    static validation_result_t failure(
        nested_text::value_t failing_predicate, value_info_t failed_object, nested_text::value_t failure_message)
    {
        return { false, { std::move(failing_predicate), std::move(failed_object), std::move(failure_message) } };
    }

    bool is_success() const { return m_success; }

    const failure_t* get_failure() const
    {
        if (is_success())
        {
            return nullptr;
        }
        return &m_failure;
    }

    friend std::ostream& operator<<(std::ostream& os, const validation_result_t& item)
    {
        if (item.is_success())
        {
            os << "success";
        }
        else
        {
            // os << "failure: " << item.get_failure()->format();
            os << "failure:\n```\n";
            nested_text::pretty_print(os, item.get_failure()->format());
            os << "```\n";
        }
        return os;
    }
};

template <class Impl>
struct predicate_interface_t
{
    Impl m_impl;

    template <class T>
    bool test(const T& value) const
    {
        return m_impl.test(value);
    }

    template <class T>
    bool operator()(const T& value) const
    {
        return test(value);
    }

    template <class T>
    validation_result_t validate(const T& value) const
    {
        return m_impl.validate(value);
    }

    nested_text::value_t format() const { return m_impl.format(); }
};

template <class Impl>
predicate_interface_t(Impl&&) -> predicate_interface_t<std::decay_t<Impl>>;

template <class T>
using has_format_impl = decltype(std::declval<T>().format());

template <class T>
struct has_format : is_detected<has_format_impl, T>
{
};

template <class T>
nested_text::value_t fmt(const T& item)
{
    if constexpr (std::is_constructible_v<nested_text::value_t, T>)
    {
        return nested_text::value_t{ item };
    }
    if constexpr (has_format<T>::value)
    {
        return item.format();
    }
    else
    {
        return nested_text::encode(item);
    }
}

template <class T>
value_info_t get_value_info(const T& item)
{
    return { fmt(item), nested_text::string_t{ type_name(item) } };
}

namespace detail
{

struct all_fn
{
    template <class... Preds>
    struct predicate_t
    {
        std::tuple<Preds...> m_preds;

        template <class T>
        bool test(const T& item) const
        {
            return std::apply([&](const auto&... pred) { return (pred.test(item) && ...); }, m_preds);
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            nested_text::list_t failed_preds;
            std::apply(
                [&](const auto&... pred)
                {
                    (std::invoke(
                         [&]()
                         {
                             const validation_result_t result = pred.validate(item);
                             if (!result.is_success())
                             {
                                 failed_preds.push_back(
                                     nested_text::map_t{ { "predicate", fmt(pred) },
                                                         { "failure_message", result.get_failure()->failure_message } });
                             }
                         }),
                     ...);
                },
                m_preds);
            if (failed_preds.empty())
            {
                return validation_result_t::success();
            }
            const auto failed_count = failed_preds.size();
            return validation_result_t::failure(
                format(),
                get_value_info(item),
                nested_text::map_t{ { "message", "some predicates did not satisfy the condition" },
                                    { "count",
                                      nested_text::map_t{ { "total", nested_text::encode(sizeof...(Preds)) },
                                                          { "failed", nested_text::encode(failed_count) } } },
                                    { "failed_predicates", std::move(failed_preds) } });
        }

        nested_text::value_t format() const
        {
            nested_text::list_t pred_formats;
            std::apply([&](const auto&... pred) { (pred_formats.push_back(fmt(pred)), ...); }, m_preds);
            return nested_text::list_t{ "all", std::move(pred_formats) };
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } } };
    }
};

struct any_fn
{
    template <class... Preds>
    struct predicate_t
    {
        std::tuple<Preds...> m_preds;

        template <class T>
        bool test(const T& item) const
        {
            return std::apply([&](const auto&... pred) { return (pred.test(item) || ...); }, m_preds);
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            bool any_matches = false;
            std::apply(
                [&](const auto&... pred) mutable
                {
                    (std::invoke(
                         [&]()
                         {
                             const validation_result_t result = pred.validate(item);
                             any_matches |= result.is_success();
                         }),
                     ...);
                },
                m_preds);
            if (any_matches)
            {
                return validation_result_t::success();
            }
            return validation_result_t::failure(
                format(),
                get_value_info(item),
                nested_text::map_t{ { "message", "no predicate satisfied the condition" },
                                    { "count",
                                      nested_text::map_t{ { "total", nested_text::encode(sizeof...(Preds)) },
                                                          { "failed", nested_text::encode(sizeof...(Preds)) } } } });
        }

        nested_text::value_t format() const
        {
            nested_text::list_t pred_formats;
            std::apply([&](const auto&... pred) { (pred_formats.push_back(fmt(pred)), ...); }, m_preds);
            return nested_text::list_t{ "any", std::move(pred_formats) };
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } } };
    }
};

struct eq_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item == m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "eq", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct ne_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item != m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "ne", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct gt_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item > m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "gt", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct lt_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item < m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "lt", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct le_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item <= m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "le", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct ge_fn
{
    template <class V>
    struct predicate_t
    {
        V m_value;

        template <class T>
        bool test(const T& item) const
        {
            return item >= m_value;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            if (test(item))
            {
                return validation_result_t::success();
            }
            else
            {
                return validation_result_t::failure(
                    format(), get_value_info(item), nested_text::list_t{ "expected", format() });
            }
        }

        nested_text::value_t format() const { return nested_text::list_t{ "ge", fmt(m_value) }; }
    };

    template <class T>
    auto operator()(T&& value) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<T>>{ std::forward<T>(value) } };
    }
};

struct each_element_fn
{
    template <class Pred>
    struct predicate_t
    {
        Pred m_pred;

        template <class T>
        bool test(const T& item) const
        {
            for (const auto& element : item)
            {
                if (!m_pred.test(element))
                {
                    return false;
                }
            }
            return true;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            nested_text::list_t failed_elements;
            std::ptrdiff_t index = 0;
            std::size_t size = 0;
            for (const auto& element : item)
            {
                const validation_result_t result = m_pred.validate(element);
                if (!result.is_success())
                {
                    failed_elements.push_back(nested_text::map_t{ { "index", nested_text::encode(index) },
                                                                  // { "value", nested_text::encode(element) },
                                                                  { "failure_message", result.get_failure()->format() } });
                }
                ++index;
                ++size;
            }
            if (failed_elements.empty())
            {
                return validation_result_t::success();
            }
            const auto failed_count = failed_elements.size();
            return validation_result_t::failure(
                format(),
                get_value_info(item),
                nested_text::map_t{ { { "message", "some elements did not satisfy the predicate" },
                                      { "count",
                                        nested_text::map_t{ { "total", nested_text::encode(size) },
                                                            { "failed", nested_text::encode(failed_count) } } },
                                      { "failed_elements", std::move(failed_elements) } } });
        }

        nested_text::value_t format() const { return nested_text::list_t{ "each_element", fmt(m_pred) }; }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct contains_element_fn
{
    template <class Pred>
    struct predicate_t
    {
        Pred m_pred;

        template <class T>
        bool test(const T& item) const
        {
            for (const auto& element : item)
            {
                if (m_pred.test(element))
                {
                    return true;
                }
            }
            return false;
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            nested_text::list_t failed_elements;
            std::size_t size = 0;
            for (const auto& element : item)
            {
                const validation_result_t result = m_pred.validate(element);
                if (result.is_success())
                {
                    return validation_result_t::success();
                }
                ++size;
            }
            return validation_result_t::failure(
                format(),
                get_value_info(item),
                nested_text::map_t{ { { "message", "no element satisfied the predicate" },
                                      { "count",
                                        nested_text::map_t{ { "total", nested_text::encode(size) },
                                                            { "failed", nested_text::encode(size) } } } } });
        }

        nested_text::value_t format() const { return nested_text::list_t{ "contains_element", fmt(m_pred) }; }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct result_of_fn
{
    template <class Func, class Pred>
    struct predicate_t
    {
        std::string m_name;
        Func m_func;
        Pred m_pred;

        template <class T>
        bool test(const T& item) const
        {
            return m_pred.test(std::invoke(m_func, item));
        }

        template <class T>
        validation_result_t validate(const T& item) const
        {
            const auto& value = std::invoke(m_func, item);
            const validation_result_t result = m_pred.validate(value);
            if (result.is_success())
            {
                return validation_result_t::success();
            }
            return validation_result_t::failure(format(), get_value_info(item), result.get_failure()->format());
        }

        nested_text::value_t format() const { return nested_text::list_t{ str(":", m_name), fmt(m_pred) }; }
    };

    template <class Func, class Pred>
    auto operator()(std::string name, Func&& func, Pred&& pred) const
    {
        return predicate_interface_t{ predicate_t<std::decay_t<Func>, std::decay_t<Pred>>{
            std::move(name), std::forward<Func>(func), std::forward<Pred>(pred) } };
    }
};

}  // namespace detail

static constexpr inline auto all = detail::all_fn{};
static constexpr inline auto any = detail::any_fn{};

static constexpr inline auto eq = detail::eq_fn{};
static constexpr inline auto ne = detail::ne_fn{};

static constexpr inline auto gt = detail::gt_fn{};
static constexpr inline auto lt = detail::lt_fn{};
static constexpr inline auto ge = detail::ge_fn{};
static constexpr inline auto le = detail::le_fn{};

static constexpr inline auto each_element = detail::each_element_fn{};
static constexpr inline auto contains_element = detail::contains_element_fn{};
static constexpr inline auto result_of = detail::result_of_fn{};

}  // namespace predicates
}  // namespace zx
