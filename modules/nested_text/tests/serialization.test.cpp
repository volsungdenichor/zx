#include <gmock/gmock.h>

#include <zx/nested_text.hpp>

TEST(nested_text_serialization, integer)
{
    EXPECT_THAT(zx::nested_text::encode(123), testing::Eq(zx::nested_text::value_t{ "123" }));
    EXPECT_THAT((zx::nested_text::decode<int>(zx::nested_text::value_t{ "123" })), testing::Eq(123));
}

TEST(nested_text_serialization, string)
{
    EXPECT_THAT(zx::nested_text::encode(std::string{ "Hello" }), testing::Eq(zx::nested_text::value_t{ "Hello" }));
    EXPECT_THAT((zx::nested_text::decode<std::string>(zx::nested_text::value_t{ "Hello" })), testing::Eq("Hello"));
}

TEST(nested_text_serialization, vector)
{
    EXPECT_THAT(
        zx::nested_text::encode(std::vector<int>{ 1, 2, 3 }),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::list_t{ "1", "2", "3" } }));
    EXPECT_THAT(
        (zx::nested_text::decode<std::vector<int>>(zx::nested_text::value_t{ zx::nested_text::list_t{ "1", "2", "3" } })),
        testing::Eq(std::vector<int>{ 1, 2, 3 }));
}