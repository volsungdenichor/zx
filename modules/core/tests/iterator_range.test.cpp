#include <gmock/gmock.h>

#include <zx/iterator_range.hpp>

TEST(iteratror_range, slicing)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7));

    EXPECT_THAT(span.take(4), testing::ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(span.drop(4), testing::ElementsAre(5, 6, 7));
    EXPECT_THAT(span.take_back(4), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span.drop_back(4), testing::ElementsAre(1, 2, 3));

    EXPECT_THAT(span.slice({ {}, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span.slice({ 0, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span.slice({ 1, 3 }), testing::ElementsAre(2, 3));
    EXPECT_THAT(span.slice({ 3, {} }), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span.slice({ 3, 5 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span.slice({ -2, {} }), testing::ElementsAre(6, 7));
    EXPECT_THAT(span.slice({ {}, -2 }), testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(span.slice({ -4, -2 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span.slice({ -2, -4 }), testing::IsEmpty());
}

TEST(iterator_range, find)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span.find(4), testing::Optional(4));
    EXPECT_THAT(span.find(-1), testing::Eq(zx::none));
}

TEST(iterator_range, find_if)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span.find_if([](int x) { return x % 2 == 0; }), testing::Optional(2));
    EXPECT_THAT(span.find_if([](int x) { return x < 0; }), testing::Eq(zx::none));
}

TEST(iterator_range, starts_with)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_TRUE(span.starts_with(std::vector{ 1, 2 }));
    EXPECT_TRUE(span.starts_with(std::vector{ 1, 2, 3 }));
    EXPECT_FALSE(span.starts_with(std::vector{ 2, 3 }));
}

TEST(iterator_range, ends_with)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_TRUE(span.ends_with(std::vector{ 6, 7 }));
    EXPECT_TRUE(span.ends_with(std::vector{ 5, 6, 7 }));
    EXPECT_FALSE(span.ends_with(std::vector{ 5, 6 }));
}

TEST(iterator_range, contains_subrange)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_TRUE(span.contains_subrange(std::vector{ 3, 4 }));
    EXPECT_TRUE(span.contains_subrange(std::vector{ 1, 2, 3 }));
    EXPECT_TRUE(span.contains_subrange(std::vector{ 5, 6, 7 }));
    EXPECT_FALSE(span.contains_subrange(std::vector{ 2, 4 }));
}

TEST(iterator_range, all_any_none_of)
{
    constexpr auto is_even = [](int x) { return x % 2 == 0; };
    constexpr auto is_positive = [](int x) { return x > 0; };
    constexpr auto is_negative = [](int x) { return x < 0; };
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };

    EXPECT_TRUE(span.all_of(is_positive));
    EXPECT_FALSE(span.all_of(is_even));
    EXPECT_TRUE(span.any_of(is_even));
    EXPECT_FALSE(span.any_of(is_negative));
    EXPECT_TRUE(span.none_of(is_negative));
    EXPECT_FALSE(span.none_of(is_even));
}
