#include <gmock/gmock.h>

#include <zx/static_string.hpp>

TEST(static_string, value)
{
    constexpr auto str = zx::static_string_t<'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'>{};

    EXPECT_THAT(str.get(), "Hello, World!");
}

TEST(static_string, comparison)
{
    using namespace std::string_view_literals;

    constexpr auto str1 = zx::static_string_t<'H', 'e', 'l', 'l', 'o'>{};
    constexpr auto str2 = zx::static_string_t<'H', 'e', 'l', 'l', 'o'>{};

    EXPECT_THAT(str1 == str2, testing::IsTrue());
    EXPECT_THAT(str1 != str2, testing::IsFalse());
    EXPECT_THAT(str1 < str2, testing::IsFalse());
    EXPECT_THAT(str1 <= str2, testing::IsTrue());
    EXPECT_THAT(str1 > str2, testing::IsFalse());
    EXPECT_THAT(str1 >= str2, testing::IsTrue());
    EXPECT_THAT(str1 <= str2, testing::IsTrue());
    EXPECT_THAT(str1 > str2, testing::IsFalse());
    EXPECT_THAT(str1 >= str2, testing::IsTrue());

    EXPECT_THAT(str1 == "Hello"sv, testing::IsTrue());
    EXPECT_THAT(str1 != "Hello"sv, testing::IsFalse());
    EXPECT_THAT(str1 < "Hello"sv, testing::IsFalse());
    EXPECT_THAT(str1 <= "Hello"sv, testing::IsTrue());
    EXPECT_THAT(str1 > "Hello"sv, testing::IsFalse());
    EXPECT_THAT(str1 >= "Hello"sv, testing::IsTrue());

    EXPECT_THAT(str1 == "World"sv, testing::IsFalse());
    EXPECT_THAT(str1 != "World"sv, testing::IsTrue());
    EXPECT_THAT(str1 < "World"sv, testing::IsTrue());
    EXPECT_THAT(str1 <= "World"sv, testing::IsTrue());
    EXPECT_THAT(str1 > "World"sv, testing::IsFalse());
    EXPECT_THAT(str1 >= "World"sv, testing::IsFalse());
}

TEST(static_string, concat)
{
    using namespace std::string_view_literals;

    constexpr auto str1 = zx::static_string_t<'H', 'e', 'l', 'l', 'o'>{};
    constexpr auto str2 = zx::static_string_t<',', ' ', 'W', 'o', 'r', 'l', 'd', '!'>{};

    using concat_type = decltype(str1 + str2);

    EXPECT_THAT(concat_type{}.get(), "Hello, World!"sv);
    EXPECT_THAT(str1 + str2, "Hello, World!"sv);
}
