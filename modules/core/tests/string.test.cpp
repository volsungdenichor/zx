#include <gmock/gmock.h>

#include <zx/string.hpp>

TEST(string, conversion_from_std_string)
{
    using g = zx::string_t::value_type;
    EXPECT_THAT(
        zx::string_t{ "❤Hello, world!🔴" },
        testing::ElementsAreArray({ g("❤"),
                                    g('H'),
                                    g('e'),
                                    g('l'),
                                    g('l'),
                                    g('o'),
                                    g(','),
                                    g(' '),
                                    g('w'),
                                    g('o'),
                                    g('r'),
                                    g('l'),
                                    g('d'),
                                    g('!'),
                                    g("🔴") }));
}
