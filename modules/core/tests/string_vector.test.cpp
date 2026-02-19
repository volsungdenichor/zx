#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <zx/string_vector.hpp>

TEST(string_vector, default_construction)
{
    zx::string_vector vec;
    EXPECT_THAT(vec, testing::AllOf(testing::SizeIs(0)));
    EXPECT_THAT(std::distance(vec.begin(), vec.end()), 0);
    EXPECT_THAT(std::distance(vec.rbegin(), vec.rend()), 0);
    EXPECT_THROW(vec.front(), std::out_of_range);
    EXPECT_THROW(vec.back(), std::out_of_range);
    EXPECT_THROW(vec[0], std::out_of_range);
    EXPECT_THROW(vec.at(0), std::out_of_range);
}

TEST(string_vector, push_back_and_iteration)
{
    zx::string_vector vec = { "Hello", "World", "!" };
    EXPECT_THAT(vec, testing::AllOf(testing::ElementsAre("Hello", "World", "!"), testing::SizeIs(3)));
    EXPECT_THAT(std::distance(vec.begin(), vec.end()), 3);
    EXPECT_THAT(std::distance(vec.rbegin(), vec.rend()), 3);
    EXPECT_THAT(vec[1], testing::StrEq("World"));
    EXPECT_THAT(vec.at(2), testing::StrEq("!"));
    EXPECT_THROW(vec[3], std::out_of_range);
    EXPECT_THAT(vec.front(), testing::StrEq("Hello"));
    EXPECT_THAT(vec.back(), testing::StrEq("!"));
}
