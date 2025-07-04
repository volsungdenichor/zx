#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <sstream>
#include <string_view>

namespace matchers
{

namespace detail
{
template <class Op>
struct compare_matcher
{
    std::string_view m_name;

    template <class T>
    struct impl : public Catch::Matchers::MatcherGenericBase
    {
        T m_expected;
        std::string_view m_name;

        impl(T expected, std::string_view name) : m_expected{ std::move(expected) }, m_name{ name }
        {
        }

        template <class U>
        bool match(U&& actual) const
        {
            return Op{}(actual, m_expected);
        }

        std::string describe() const override
        {
            std::stringstream ss;
            ss << m_name << " " << m_expected;
            return ss.str();
        }
    };

    template <class T>
    auto operator()(T expected) const -> impl<T>
    {
        return impl<T>{ std::move(expected), m_name };
    }
};

struct elements_are_matcher
{
    template <class T, class... Tail>
    auto operator()(T head, Tail&&... tail) const
    {
        std::vector<T> v = { head, std::forward<Tail>(tail)... };
        return Catch::Matchers::RangeEquals(std::move(v));
    }
};

struct is_empty_matcher
{
    auto operator()() const
    {
        return Catch::Matchers::IsEmpty();
    }
};

struct size_is_matcher
{
    template <class T>
    auto operator()(T matcher) const
    {
        return Catch::Matchers::SizeIs(std::move(matcher));
    }
};

}  // namespace detail

static constexpr inline auto equal_to = detail::compare_matcher<std::equal_to<>>{ "equal to" };
static constexpr inline auto not_equal_to = detail::compare_matcher<std::not_equal_to<>>{ "not equal to" };
static constexpr inline auto less = detail::compare_matcher<std::less<>>{ "less than" };
static constexpr inline auto greater = detail::compare_matcher<std::greater<>>{ "greater than" };
static constexpr inline auto less_equal = detail::compare_matcher<std::less_equal<>>{ "less than or equal to" };
static constexpr inline auto greater_equal = detail::compare_matcher<std::greater_equal<>>{ "greater than or equal to" };

static constexpr inline auto elements_are = detail::elements_are_matcher{};
static constexpr inline auto is_empty = detail::is_empty_matcher{};
static constexpr inline auto size_is = detail::size_is_matcher{};

}  // namespace matchers
