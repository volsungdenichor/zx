#include <gmock/gmock.h>

#include <zx/range.hpp>

TEST(range, basic)
{
    const auto r = zx::range(3, 7);
    EXPECT_THAT(r.size(), testing::Eq(4));
    EXPECT_THAT(r.front(), testing::Eq(3));
    EXPECT_THAT(r.back(), testing::Eq(6));
    EXPECT_THAT(r, testing::ElementsAre(3, 4, 5, 6));
}

TEST(range, from_zero)
{
    const auto r = zx::range(7);
    EXPECT_THAT(r.size(), testing::Eq(7));
    EXPECT_THAT(r.front(), testing::Eq(0));
    EXPECT_THAT(r.back(), testing::Eq(6));
    EXPECT_THAT(r, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}