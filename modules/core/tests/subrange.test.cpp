#include <gmock/gmock.h>

#include <zx/subrange.hpp>

TEST(subrange, slicing)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7));

    EXPECT_THAT(span |= zx::subrange::take(4), testing::ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(span |= zx::subrange::drop(4), testing::ElementsAre(5, 6, 7));
    EXPECT_THAT(span |= zx::subrange::take_back(4), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span |= zx::subrange::drop_back(4), testing::ElementsAre(1, 2, 3));

    EXPECT_THAT(span |= zx::subrange::slice({ {}, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span |= zx::subrange::slice({ 0, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span |= zx::subrange::slice({ 1, 3 }), testing::ElementsAre(2, 3));
    EXPECT_THAT(span |= zx::subrange::slice({ 3, {} }), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span |= zx::subrange::slice({ 3, 5 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span |= zx::subrange::slice({ -2, {} }), testing::ElementsAre(6, 7));
    EXPECT_THAT(span |= zx::subrange::slice({ {}, -2 }), testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(span |= zx::subrange::slice({ -4, -2 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span |= zx::subrange::slice({ -2, -4 }), testing::IsEmpty());
}

TEST(subrange, take_drop_while)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_THAT(span |= zx::subrange::take_while([](int x) { return x < 4; }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span |= zx::subrange::drop_while([](int x) { return x < 4; }), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span |= zx::subrange::take_back_while([](int x) { return x > 4; }), testing::ElementsAre(5, 6, 7));
    EXPECT_THAT(span |= zx::subrange::drop_back_while([](int x) { return x > 4; }), testing::ElementsAre(1, 2, 3, 4));
}

TEST(subrange, at)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_THAT(zx::subrange::at(span, 2), 3);
    EXPECT_THAT(span |= zx::subrange::at(2), 3);
    EXPECT_THROW(zx::subrange::at(span, 7), std::out_of_range);
    EXPECT_THROW((span |= zx::subrange::at(7)), std::out_of_range);
}

TEST(subrange, maybe_at)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_THAT(zx::subrange::maybe_at(span, 2), testing::Optional(3));
    EXPECT_THAT(span |= zx::subrange::maybe_at(2), testing::Optional(3));
    EXPECT_THAT(zx::subrange::maybe_at(span, 7), testing::Eq(zx::none));
    EXPECT_THAT(span |= zx::subrange::maybe_at(7), testing::Eq(zx::none));
}

TEST(subrange, find)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(
        span |= zx::subrange::find(4),
        testing::AllOf(
            testing::ResultOf(
                "from", [](auto c) { return c.from(); }, testing::ElementsAre(4, 5, 6, 7)),
            testing::ResultOf(
                "found", [](auto c) { return c.found(); }, testing::ElementsAre(4)),
            testing::ResultOf(
                "before", [](auto c) { return c.before(); }, testing::ElementsAre(1, 2, 3)),
            testing::ResultOf("after", [](auto c) { return c.after(); }, testing::ElementsAre(5, 6, 7))));
    EXPECT_THAT(
        span |= zx::subrange::find(-1),
        testing::AllOf(
            testing::ResultOf(
                "from", [](auto c) { return c.from(); }, testing::IsEmpty()),
            testing::ResultOf(
                "found", [](auto c) { return c.found(); }, testing::IsEmpty()),
            testing::ResultOf(
                "before", [](auto c) { return c.before(); }, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7)),
            testing::ResultOf("after", [](auto c) { return c.after(); }, testing::IsEmpty())));
}

TEST(subrange, find_if)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(
        span |= zx::subrange::find_if([](int x) { return x % 2 == 0; }),
        testing::AllOf(
            testing::ResultOf(
                "from", [](auto c) { return c.from(); }, testing::ElementsAre(2, 3, 4, 5, 6, 7)),
            testing::ResultOf(
                "found", [](auto c) { return c.found(); }, testing::ElementsAre(2)),
            testing::ResultOf(
                "before", [](auto c) { return c.before(); }, testing::ElementsAre(1)),
            testing::ResultOf("after", [](auto c) { return c.after(); }, testing::ElementsAre(3, 4, 5, 6, 7))));
    EXPECT_THAT(
        span |= zx::subrange::find_if([](int x) { return x < 0; }),
        testing::AllOf(
            testing::ResultOf(
                "from", [](auto c) { return c.from(); }, testing::IsEmpty()),
            testing::ResultOf(
                "found", [](auto c) { return c.found(); }, testing::IsEmpty()),
            testing::ResultOf(
                "before", [](auto c) { return c.before(); }, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7)),
            testing::ResultOf("after", [](auto c) { return c.after(); }, testing::IsEmpty())));
}

TEST(subrange, starts_with)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span.starts_with(std::vector{ 1, 2 }), testing::IsTrue());
    EXPECT_THAT(span.starts_with(std::vector{ 1, 2, 3 }), testing::IsTrue());
    EXPECT_THAT(span.starts_with(std::vector{ 2, 3 }), testing::IsFalse());
}

TEST(subrange, ends_with)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span.ends_with(std::vector{ 6, 7 }), testing::IsTrue());
    EXPECT_THAT(span.ends_with(std::vector{ 5, 6, 7 }), testing::IsTrue());
    EXPECT_THAT(span.ends_with(std::vector{ 5, 6 }), testing::IsFalse());
}

TEST(subrange, contains_subrange)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span.contains_subrange(std::vector{ 3, 4 }), testing::IsTrue());
    EXPECT_THAT(span.contains_subrange(std::vector{ 1, 2, 3 }), testing::IsTrue());
    EXPECT_THAT(span.contains_subrange(std::vector{ 5, 6, 7 }), testing::IsTrue());
    EXPECT_THAT(span.contains_subrange(std::vector{ 2, 4 }), testing::IsFalse());
}

TEST(subrange, all_any_none_of)
{
    constexpr auto is_even = [](int x) { return x % 2 == 0; };
    constexpr auto is_positive = [](int x) { return x > 0; };
    constexpr auto is_negative = [](int x) { return x < 0; };
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_THAT(span |= zx::subrange::all_of(is_positive), testing::IsTrue());
    EXPECT_THAT(span |= zx::subrange::all_of(is_even), testing::IsFalse());
    EXPECT_THAT(span |= zx::subrange::any_of(is_even), testing::IsTrue());
    EXPECT_THAT(span |= zx::subrange::any_of(is_negative), testing::IsFalse());
    EXPECT_THAT(span |= zx::subrange::none_of(is_negative), testing::IsTrue());
    EXPECT_THAT(span |= zx::subrange::none_of(is_even), testing::IsFalse());
}

TEST(subrange, comparison_operators)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_THAT(span == (std::vector{ 1, 2, 3, 4, 5, 6, 7 }), testing::IsTrue());
    EXPECT_THAT(span == (std::vector{ 1, 2 }), testing::IsFalse());
    EXPECT_THAT(span != (std::vector{ 1, 2 }), testing::IsTrue());
    EXPECT_THAT(span != (std::vector{ 1, 2, 3, 4, 5, 6, 7 }), testing::IsFalse());

    EXPECT_THAT(span < (std::vector{ 2 }), testing::IsTrue());
    EXPECT_THAT(span < (std::vector{ 1 }), testing::IsFalse());
    EXPECT_THAT(span < (std::vector{ 1, 2, 3, 4, 5, 6 }), testing::IsFalse());
    EXPECT_THAT(span < (std::vector{ 1, 2, 3, 4, 5, 6, 7, 8 }), testing::IsTrue());

    EXPECT_THAT(span > (std::vector<int>{}), testing::IsTrue());
    EXPECT_THAT(span > (std::vector{ 1 }), testing::IsTrue());
    EXPECT_THAT(span > (std::vector{ 1, 2 }), testing::IsTrue());
    EXPECT_THAT(span > (std::vector{ 0 }), testing::IsTrue());

    EXPECT_THAT(span <= (std::vector{ 1, 2, 3, 4, 5, 6, 7 }), testing::IsTrue());
    EXPECT_THAT(span <= (std::vector{ 1, 2 }), testing::IsFalse());
    EXPECT_THAT(span <= (std::vector{ 1, 2, 3, 4, 5, 6 }), testing::IsFalse());
    EXPECT_THAT(span <= (std::vector{ 1, 2, 3, 4, 5, 6, 7, 8 }), testing::IsTrue());
}