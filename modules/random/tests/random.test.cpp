#include <gmock/gmock.h>

#include <zx/random.hpp>

TEST(random, uniform)
{
    const auto f = zx::random::uniform(0, 10);
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_THAT(f(), testing::AllOf(testing::Ge(0), testing::Le(10)));
    }
}

TEST(random, normal)
{
    const auto first = zx::random::normal(2.0, 0.5, zx::random::seed_t{ 123 });
    const auto second = zx::random::normal(2.0, 0.5, zx::random::seed_t{ 123 });

    static_assert(std::is_same_v<decltype(first()), double>);
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_THAT(first(), second());
    }
}

TEST(random, invoke)
{
    const auto f = zx::random::invoke(
        [](auto x, auto y) {
            return std::pair{ x, y };
        },
        zx::random::uniform(0, 3),
        zx::random::uniform(0, 5));
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_THAT(
            f(),
            testing::Pair(testing::AllOf(testing::Ge(0), testing::Le(3)), testing::AllOf(testing::Ge(0), testing::Le(5))));
    }
}