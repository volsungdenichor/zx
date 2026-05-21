#include <gmock/gmock.h>

#include <zx/predicates.hpp>

namespace
{

constexpr auto Expected = [](auto&& matcher)
{
    return testing::Field(
        "expected", &zx::predicates::validation_error_t::expected, std::forward<decltype(matcher)>(matcher));
};

constexpr auto Actual = [](auto&& matcher)
{ return testing::Field("actual", &zx::predicates::validation_error_t::actual, std::forward<decltype(matcher)>(matcher)); };

constexpr auto Reason = [](auto&& matcher)
{ return testing::Field("reason", &zx::predicates::validation_error_t::reason, std::forward<decltype(matcher)>(matcher)); };

struct NestedTextEqualsFn
{
    struct Matcher
    {
        using is_gtest_matcher = void;
        zx::nested_text::node_t m_expected;

        bool MatchAndExplain(const zx::nested_text::node_t& actual, std::ostream* os) const
        {
            const zx::nested_text::list_t errors = zx::nested_text::compare(m_expected, actual, "expected", "actual");
            if (errors.empty())
            {
                return true;
            }
            else
            {
                if (os)
                {
                    *os << "expected:\n" << m_expected << "\nactual:\n" << actual << "\n";
                    *os << "differences:\n";
                    for (const zx::nested_text::node_t& error : errors)
                    {
                        *os << error << "\n";
                    }
                }
                return false;
            }
        }

        void DescribeTo(std::ostream* os) const
        {
            if (os)
            {
                *os << "equals " << m_expected;
            }
        }

        void DescribeNegationTo(std::ostream* os) const
        {
            if (os)
            {
                *os << "does not equal " << m_expected;
            }
        }
    };

    auto operator()(zx::nested_text::node_t expected) const -> testing::Matcher<const zx::nested_text::node_t&>
    {
        return Matcher{ std::move(expected) };
    }
};

static constexpr inline auto NestedTextEquals = NestedTextEqualsFn{};

}  // namespace

TEST(predicates, eq)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::eq(42);
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([eq 42])"_node));
    EXPECT_THAT(pred.test(42), true);
    EXPECT_THAT(pred.test(43), false);
    EXPECT_THAT(pred(42), true);
    EXPECT_THAT(pred(43), false);
    EXPECT_THAT(pred.validate(42).is_success(), true);
    EXPECT_THAT(pred.validate(43).is_success(), false);

    EXPECT_THAT(
        pred.validate(43).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([eq 42])"_node)),
            Actual(NestedTextEquals(R"(43)"_node)),
            Reason(NestedTextEquals(R"("value is not equal to expected")"_node))));
}

TEST(predicates, all)
{
    using namespace zx::nested_text::literals;

    const auto pred
        = zx::predicates::all(zx::predicates::ge(1000), zx::predicates::lt(1100), zx::predicates::divisible_by(7));
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([all [[ge 1000] [lt 1100] [divisible_by 7]]])"_node));

    EXPECT_THAT(pred(0), false);
    EXPECT_THAT(pred(100), false);
    EXPECT_THAT(pred(1000), false);
    EXPECT_THAT(pred(1001), true);
    EXPECT_THAT(pred(1099), true);
    EXPECT_THAT(pred(1100), false);

    EXPECT_THAT(pred.validate(0).is_success(), false);
    EXPECT_THAT(pred.validate(100).is_success(), false);
    EXPECT_THAT(pred.validate(1000).is_success(), false);
    EXPECT_THAT(pred.validate(1001).is_success(), true);
    EXPECT_THAT(pred.validate(1099).is_success(), true);
    EXPECT_THAT(pred.validate(1100).is_success(), false);

    EXPECT_THAT(
        pred.validate(55).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([all [[ge 1000] [lt 1100] [divisible_by 7]]])"_node)),
            Actual(NestedTextEquals(R"(55)"_node)),
            Reason(NestedTextEquals(
                R"({:failing_predicates [
                    {:predicate [ge 1000] :reason "value is not greater than or equal to expected"}
                    {:predicate [divisible_by 7] :reason "value is not divisible by 7"}]})"_node))));
}

TEST(predicates, any)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::any(100, zx::predicates::all(zx::predicates::gt(1000), zx::predicates::lt(1100)));
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([any [100 [all [[gt 1000] [lt 1100]]]]])"_node));
    EXPECT_THAT(pred(0), false);
    EXPECT_THAT(pred(100), true);
    EXPECT_THAT(pred(1000), false);
    EXPECT_THAT(pred(1001), true);
    EXPECT_THAT(pred(1099), true);
    EXPECT_THAT(pred(1100), false);

    EXPECT_THAT(pred.validate(0).is_success(), false);
    EXPECT_THAT(pred.validate(100).is_success(), true);
    EXPECT_THAT(pred.validate(1000).is_success(), false);
    EXPECT_THAT(pred.validate(1001).is_success(), true);
    EXPECT_THAT(pred.validate(1099).is_success(), true);
    EXPECT_THAT(pred.validate(1100).is_success(), false);

    EXPECT_THAT(
        pred.validate(55).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([any [100 [all [[gt 1000] [lt 1100]]]]])"_node)),
            Actual(NestedTextEquals(R"(55)"_node)),
            Reason(NestedTextEquals(R"(["none of the predicates matched"])"_node))));
}

TEST(predicates, is_not)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::is_not(42);
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([is_not 42])"_node));
    EXPECT_THAT(pred.test(42), false);
    EXPECT_THAT(pred.test(43), true);
    EXPECT_THAT(pred(42), false);
    EXPECT_THAT(pred(43), true);
    EXPECT_THAT(pred.validate(42).is_success(), false);
    EXPECT_THAT(pred.validate(43).is_success(), true);

    EXPECT_THAT(
        pred.validate(42).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([is_not 42])"_node)),
            Actual(NestedTextEquals(R"(42)"_node)),
            Reason(NestedTextEquals(R"("predicate matched but was expected not to")"_node))));
}

TEST(predicates, items_any)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::items_any(3);
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([items_any 3])"_node));
    EXPECT_THAT(pred(std::vector<int>{}), false);
    EXPECT_THAT(pred(std::vector{ -1, -2, -3 }), false);
    EXPECT_THAT(pred(std::vector{ -1, 0, -3 }), false);
    EXPECT_THAT(pred(std::vector{ -1, 0, 3 }), true);

    EXPECT_THAT(pred.validate(std::vector{ -1, 0, -3 }).is_success(), false);
    EXPECT_THAT(
        pred.validate(std::vector{ -1, 0, -3 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_any 3])"_node)),
            Actual(NestedTextEquals(R"([-1 0 -3])"_node)),
            Reason(NestedTextEquals(R"("no items in the range matched the predicate")"_node))));
}

TEST(predicates, items_none)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::items_none(zx::predicates::gt(0));
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([items_none [gt 0]])"_node));
    EXPECT_THAT(pred(std::vector<int>{}), true);
    EXPECT_THAT(pred(std::vector{ -1, -2, -3 }), true);
    EXPECT_THAT(pred(std::vector{ -1, 0, -3 }), true);
    EXPECT_THAT(pred(std::vector{ -1, 0, 3 }), false);

    EXPECT_THAT(pred.validate(std::vector{ -1, 0, 3 }).is_success(), false);
    EXPECT_THAT(
        pred.validate(std::vector{ -1, 0, 3 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_none [gt 0]])"_node)),
            Actual(NestedTextEquals(R"([-1 0 3])"_node)),
            Reason(NestedTextEquals(R"({:matching_items [{:index 2 :value 3}]})"_node))));
}

TEST(predicates, items_all)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::items_all(zx::predicates::gt(0));
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([items_all [gt 0]])"_node));
    EXPECT_THAT(pred(std::vector<int>{}), true);
    EXPECT_THAT(pred(std::vector{ 1, 2, 3 }), true);
    EXPECT_THAT(pred(std::vector{ -1, 0, 1 }), false);

    EXPECT_THAT(pred.validate(std::vector{ -1, 0, 1, 9 }).is_success(), false);
    EXPECT_THAT(
        pred.validate(std::vector{ -1, 0, 1, 9 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_all [gt 0]])"_node)),
            Actual(NestedTextEquals(R"([-1 0 1 9])"_node)),
            Reason(NestedTextEquals(R"({:failing_items [
                {:index 0 :value -1 :reason "value is not greater than expected"}
                {:index 1 :value 0 :reason "value is not greater than expected"}]})"_node))));
}

TEST(predicates, items_count)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::items_count(zx::predicates::all(zx::predicates::ge(3), zx::predicates::lt(5)));
    EXPECT_THAT(pred.format(), NestedTextEquals(R"([items_count [all [[ge 3] [lt 5]]]])"_node));
    EXPECT_THAT(pred(std::vector<int>{}), false);
    EXPECT_THAT(pred(std::vector{ 1, 2 }), false);
    EXPECT_THAT(pred(std::vector{ 1, 2, 3 }), true);
    EXPECT_THAT(pred(std::vector{ 1, 2, 3, 4 }), true);
    EXPECT_THAT(pred(std::vector{ 1, 2, 3, 4, 5 }), false);

    EXPECT_THAT(pred.validate(std::vector{ 1, 2 }).is_success(), false);
    EXPECT_THAT(
        pred.validate(std::vector{ 1, 2 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_count [all [[ge 3] [lt 5]]]])"_node)),
            Actual(NestedTextEquals(R"([1 2])"_node)),
            Reason(NestedTextEquals(
                R"(["number of items in the range does not match the expected count" {:expected_count [all [[ge 3] [lt 5]]] :actual_count 2}])"_node))));
}

TEST(predicates, items_are)
{
    using namespace zx::nested_text::literals;

    const auto pred = zx::predicates::items_are(5, zx::predicates::ge(10), zx::predicates::divisible_by(3));

    EXPECT_THAT(pred.format(), NestedTextEquals(R"([items_are [5 [ge 10] [divisible_by 3]]])"_node));
    EXPECT_THAT(pred(std::vector{ 5, 11, 21 }), true);
    EXPECT_THAT(pred(std::vector{ 5, 100, 6 }), true);
    EXPECT_THAT(pred(std::vector{ 5, 11, 22 }), false);
    EXPECT_THAT(pred(std::vector{ 5, 11, 21, 0 }), false);
    EXPECT_THAT(pred(std::vector{ 5, 11 }), false);

    EXPECT_THAT(pred.validate(std::vector{ 5, 11, 21 }).is_success(), true);
    EXPECT_THAT(pred.validate(std::vector{ 5, 100, 6 }).is_success(), true);
    EXPECT_THAT(pred.validate(std::vector{ 5, 11, 22 }).is_success(), false);
    EXPECT_THAT(pred.validate(std::vector{ 5, 11, 21, 0 }).is_success(), false);
    EXPECT_THAT(pred.validate(std::vector{ 5, 9 }).is_success(), false);

    EXPECT_THAT(
        pred.validate(std::vector{ 5, 11, 22 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_are [5 [ge 10] [divisible_by 3]]])"_node)),
            Actual(NestedTextEquals(R"([5 11 22])"_node)),
            Reason(NestedTextEquals(R"({:failing_items [
                {:index 2 :value 22 :reason "value is not divisible by 3"}]})"_node))));

    EXPECT_THAT(
        pred.validate(std::vector{ 5, 11, 21, 0 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_are [5 [ge 10] [divisible_by 3]]])"_node)),
            Actual(NestedTextEquals(R"([5 11 21 0])"_node)),
            Reason(NestedTextEquals(R"({:expected_count 3 :actual_count 4 :failing_items []})"_node))));

    EXPECT_THAT(
        pred.validate(std::vector{ 5, 9 }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([items_are [5 [ge 10] [divisible_by 3]]])"_node)),
            Actual(NestedTextEquals(R"([5 9])"_node)),
            Reason(NestedTextEquals(
                R"({:expected_count 3 :actual_count 2 :failing_items [{:index 1 :value 9 :reason "value is not greater than or equal to expected"}]})"_node))));
}

namespace
{

struct TestStruct
{
    int a;
    std::string b;
};

}  // namespace

template <>
struct zx::nested_text::codec_t<TestStruct> : zx::nested_text::struct_codec_t<TestStruct>
{
    using _ = TestStruct;
#define MEMBER(name) member_t{ #name, &_::name }
    codec_t() : struct_codec_t({ MEMBER(a), MEMBER(b) }) { }
#undef MEMBER
};

TEST(predicates, field)
{
    using namespace zx::nested_text::literals;
    using namespace std::string_literals;

    const auto pred = zx::predicates::all(
        zx::predicates::field("a", &TestStruct::a, zx::predicates::ge(10)),
        zx::predicates::field("b", &TestStruct::b, zx::predicates::ne("x"s)));

    EXPECT_THAT(pred.format(), NestedTextEquals(R"([all [[result_of a [ge 10]] [result_of b [ne x]]]])"_node));
    EXPECT_THAT(pred(TestStruct{ 5, "x" }), false);
    EXPECT_THAT(pred(TestStruct{ 5, "hello" }), false);
    EXPECT_THAT(pred(TestStruct{ 10, "x" }), false);
    EXPECT_THAT(pred(TestStruct{ 10, "hello" }), true);

    EXPECT_THAT(
        pred.validate(TestStruct{ 5, "" }).error(),
        testing::AllOf(
            Expected(NestedTextEquals(R"([all [[result_of a [ge 10]] [result_of b [ne x]]]])"_node)),
            Actual(NestedTextEquals(R"({:a 5 :b ""})"_node)),
            Reason(NestedTextEquals(
                R"({:failing_predicates [{:predicate [result_of a [ge 10]] :reason ["result of the function did not satisfy the predicate" {:function_name a :function_result 5 :predicate_error {:expected [ge 10] :actual 5 :reason "value is not greater than or equal to expected"}}]}]})"_node))));
}
