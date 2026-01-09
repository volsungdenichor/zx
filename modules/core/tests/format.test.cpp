#include <gmock/gmock.h>

#include <zx/format.hpp>

TEST(format, format_basic_types)
{
    EXPECT_THAT(zx::format(42), "42");
    EXPECT_THAT(zx::format(3.14), "3.14");
    EXPECT_THAT(zx::format(true), "true");
    EXPECT_THAT(zx::format(std::pair{ true, 42 }), "(true, 42)");
    EXPECT_THAT(zx::format(std::tuple{ 'x', true, 42 }), "(x, true, 42)");
}

TEST(format, format_multiple_arguments)
{
    EXPECT_THAT(zx::format("Value: ", 42, ", Pi: ", 3.14), "Value: 42, Pi: 3.14");
}

TEST(format, delimited_range_printing)
{
    EXPECT_THAT(zx::format(zx::delimit(std::vector<int>{ 1, 2, 3, 4, 5 }, "-")), "1-2-3-4-5");
    EXPECT_THAT(zx::format(zx::delimit(std::vector<int>{}, "-")), "");

    EXPECT_THAT(zx::format(zx::delimit("", "-")), "");
    EXPECT_THAT(zx::format(zx::delimit("ABC", "-")), "A-B-C");
}
