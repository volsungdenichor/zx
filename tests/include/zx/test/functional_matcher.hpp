#pragma once

#include <gmock/gmock.h>

#include <functional>
#include <optional>
#include <sstream>

namespace zx
{

namespace test
{

template <class Matcher>
std::string format_matcher(const Matcher& matcher, bool positive = true)
{
    std::ostringstream os;
    if (positive)
    {
        matcher.DescribeTo(&os);
    }
    else
    {
        matcher.DescribeNegationTo(&os);
    }
    return os.str();
}

struct GtestValidationResult
{
    std::string expected;
    std::string message;

    friend std::ostream& operator<<(std::ostream& os, const GtestValidationResult& item)
    {
        return os << "\n  expected: " << item.expected << "\n  message: " << item.message;
    }
};

template <class T, class Matcher>
auto validate(const T& actual, const Matcher& matcher) -> std::optional<GtestValidationResult>
{
    testing::StringMatchResultListener listener;
    const auto m = testing::SafeMatcherCast<const T&>(matcher);
    if (!testing::ExplainMatchResult(m, actual, &listener))
    {
        return GtestValidationResult{ format_matcher(m, true), listener.str() };
    }
    return std::nullopt;
}

template <class ValidateFn, class DescribeFn, class... Expected>
class FunctionalMatcher
{
public:
    using is_gtest_matcher = void;

    constexpr FunctionalMatcher(ValidateFn validate, DescribeFn describe, Expected... expected)
        : m_validate{ std::move(validate) }
        , m_describe{ std::move(describe) }
        , m_expected{ std::make_tuple(std::move(expected)...) }
    {
    }

    template <class T>
    bool MatchAndExplain(const T& actual, std::ostream* os) const
    {
        const auto mismatch = std::apply([&](const auto&... e) { return m_validate(actual, e...); }, m_expected);
        if (mismatch.has_value())
        {
            if (os)
            {
                *os << *mismatch;
            }
            return false;
        }
        return true;
    }

    void DescribeTo(std::ostream* os) const
    {
        if (os)
        {
            std::apply([&](const auto&... e) { m_describe(*os, true, e...); }, m_expected);
        }
    }
    void DescribeNegationTo(std::ostream* os) const
    {
        if (os)
        {
            std::apply([&](const auto&... e) { m_describe(*os, false, e...); }, m_expected);
        }
    }

    ValidateFn m_validate;
    DescribeFn m_describe;
    std::tuple<Expected...> m_expected;
};

}  // namespace test

}  // namespace zx
