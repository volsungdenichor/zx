#include <gmock/gmock.h>

#include <zx/yield.hpp>

TEST(yield, from)
{
    const std::vector<int> in{ 2, 3, 5, 7, 11, 13, 17, 19 };
    std::vector<int> out = {};
    zx::from(in).yield_to(zx::copy_to(std::back_inserter(out)));
    EXPECT_THAT(out, testing::ElementsAre(2, 3, 5, 7, 11, 13, 17, 19));
}

TEST(yield, into)
{
    const auto out = zx::range(1, 10).yield_to(
        zx::transduce(zx::transform([](int x) { return x * (x + 1); }), zx::into(std::vector<int>{})));
    EXPECT_THAT(out, testing::ElementsAre(2, 6, 12, 20, 30, 42, 56, 72, 90));
}

TEST(yield, range)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(zx::copy_to(std::back_inserter(out)));
    EXPECT_THAT(out, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(yield, iota)
{
    std::vector<int> out = {};
    zx::iota(1).yield_to(zx::transduce(zx::take(4), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(1, 2, 3, 4));
}

TEST(yield, all_of)
{
    bool result = zx::range(1, 10).yield_to(zx::all_of([](int x) { return x < 10; }));
    EXPECT_TRUE(result);
}

TEST(yield, any_of)
{
    bool result = zx::range(1, 10).yield_to(zx::any_of([](int x) { return x == 5; }));
    EXPECT_TRUE(result);
}

TEST(yield, none_of)
{
    bool result = zx::range(1, 10).yield_to(zx::none_of([](int x) { return x > 10; }));
    EXPECT_TRUE(result);
}

TEST(yield, transform)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(
        zx::transduce(zx::transform([](int x) { return x * x; }), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(1, 4, 9, 16, 25, 36, 49, 64, 81));
}

TEST(yield, filter)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(
        zx::transduce(zx::filter([](int x) { return x % 2 == 0; }), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(2, 4, 6, 8));
}

TEST(yield, take_while)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(
        zx::transduce(zx::take_while([](int x) { return x < 5; }), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(1, 2, 3, 4));
}

TEST(yield, drop_while)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(
        zx::transduce(zx::drop_while([](int x) { return x < 5; }), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(5, 6, 7, 8, 9));
}

TEST(yield, take)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(zx::transduce(zx::take(5), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(yield, drop)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(zx::transduce(zx::drop(5), zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(6, 7, 8, 9));
}

TEST(yield, filter_transform)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(zx::transduce(
        zx::filter([](int x) { return x % 2 == 0; }),
        zx::transform([](int x) { return x * x; }),
        zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(4, 16, 36, 64));
}

TEST(yield, transform_filter)
{
    std::vector<int> out = {};
    zx::range(1, 10).yield_to(zx::transduce(
        zx::transform([](int x) { return x * x; }),
        zx::filter([](int x) { return x % 2 == 0; }),
        zx::copy_to(std::back_inserter(out))));
    EXPECT_THAT(out, testing::ElementsAre(4, 16, 36, 64));
}

TEST(yield, fork)
{
    std::vector<int> out = {};
    const auto [_, count, sum]
        = zx::range(1, 10).yield_to(zx::fork(zx::copy_to(std::back_inserter(out)), zx::count(), zx::sum(0)));
    EXPECT_THAT(out, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
    EXPECT_THAT(sum, 45);
    EXPECT_THAT(count, 9);
}

TEST(yield, partition)
{
    std::vector<int> evens = {};
    std::vector<int> odds = {};
    zx::range(1, 10).yield_to(zx::partition(
        [](int x) { return x % 2 == 0; }, zx::copy_to(std::back_inserter(evens)), zx::copy_to(std::back_inserter(odds))));
    EXPECT_THAT(evens, testing::ElementsAre(2, 4, 6, 8));
    EXPECT_THAT(odds, testing::ElementsAre(1, 3, 5, 7, 9));
}