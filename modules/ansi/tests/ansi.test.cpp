#include <gmock/gmock.h>

#include <zx/ansi.hpp>

TEST(ansi, parse_color_names)
{
    EXPECT_THAT(zx::ansi::color_t::parse("black"), testing::Optional(zx::ansi::color_t{ 0 }));
    EXPECT_THAT(zx::ansi::color_t::parse("red"), testing::Optional(zx::ansi::color_t{ 1 }));
    EXPECT_THAT(zx::ansi::color_t::parse("green"), testing::Optional(zx::ansi::color_t{ 2 }));
    EXPECT_THAT(zx::ansi::color_t::parse("yellow"), testing::Optional(zx::ansi::color_t{ 3 }));
    EXPECT_THAT(zx::ansi::color_t::parse("blue"), testing::Optional(zx::ansi::color_t{ 4 }));
    EXPECT_THAT(zx::ansi::color_t::parse("magenta"), testing::Optional(zx::ansi::color_t{ 5 }));
    EXPECT_THAT(zx::ansi::color_t::parse("cyan"), testing::Optional(zx::ansi::color_t{ 6 }));
    EXPECT_THAT(zx::ansi::color_t::parse("white"), testing::Optional(zx::ansi::color_t{ 7 }));

    EXPECT_THAT(zx::ansi::color_t::parse("bright_black"), testing::Optional(zx::ansi::color_t{ 8 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_red"), testing::Optional(zx::ansi::color_t{ 9 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_green"), testing::Optional(zx::ansi::color_t{ 10 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_yellow"), testing::Optional(zx::ansi::color_t{ 11 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_blue"), testing::Optional(zx::ansi::color_t{ 12 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_magenta"), testing::Optional(zx::ansi::color_t{ 13 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_cyan"), testing::Optional(zx::ansi::color_t{ 14 }));
    EXPECT_THAT(zx::ansi::color_t::parse("bright_white"), testing::Optional(zx::ansi::color_t{ 15 }));
}

TEST(ansi, parse_color_rgb)
{
    EXPECT_THAT(zx::ansi::color_t::parse("0x000000"), testing::Optional(zx::ansi::color_t{ 16 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0xFF0000"), testing::Optional(zx::ansi::color_t{ 196 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x00FF00"), testing::Optional(zx::ansi::color_t{ 46 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x0000FF"), testing::Optional(zx::ansi::color_t{ 21 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0xFFFF00"), testing::Optional(zx::ansi::color_t{ 226 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0xFF00FF"), testing::Optional(zx::ansi::color_t{ 201 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x00FFFF"), testing::Optional(zx::ansi::color_t{ 51 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0xFFFFFF"), testing::Optional(zx::ansi::color_t{ 231 }));

    EXPECT_THAT(zx::ansi::color_t::parse("0x800000"), testing::Optional(zx::ansi::color_t{ 124 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x008000"), testing::Optional(zx::ansi::color_t{ 34 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x000080"), testing::Optional(zx::ansi::color_t{ 19 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x808000"), testing::Optional(zx::ansi::color_t{ 142 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x800080"), testing::Optional(zx::ansi::color_t{ 127 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x008080"), testing::Optional(zx::ansi::color_t{ 37 }));
    EXPECT_THAT(zx::ansi::color_t::parse("0x808080"), testing::Optional(zx::ansi::color_t{ 145 }));
}

TEST(ansi, parse_color_grayscale)
{
    EXPECT_THAT(zx::ansi::color_t::parse("gray:0"), testing::Optional(zx::ansi::color_t{ 232 }));
    EXPECT_THAT(zx::ansi::color_t::parse("grey:128"), testing::Optional(zx::ansi::color_t{ 244 }));
    EXPECT_THAT(zx::ansi::color_t::parse("gray:255"), testing::Optional(zx::ansi::color_t{ 255 }));
}

TEST(ansi, parse_color_invalid)
{
    EXPECT_THAT(zx::ansi::color_t::parse("invalid"), testing::Eq(std::nullopt));
    EXPECT_THAT(zx::ansi::color_t::parse("0xGGGGGG"), testing::Eq(std::nullopt));
    EXPECT_THAT(zx::ansi::color_t::parse("gray:-1"), testing::Eq(std::nullopt));
    EXPECT_THAT(zx::ansi::color_t::parse("gray:256"), testing::Eq(std::nullopt));
}

TEST(ansi, parse_font)
{
    EXPECT_THAT(zx::ansi::font_t::parse("bold"), testing::Optional(zx::ansi::font_t::bold));
    EXPECT_THAT(zx::ansi::font_t::parse("italic"), testing::Optional(zx::ansi::font_t::italic));
    EXPECT_THAT(zx::ansi::font_t::parse("underlined"), testing::Optional(zx::ansi::font_t::underlined));
    EXPECT_THAT(
        zx::ansi::font_t::parse("bold+italic"), testing::Optional(zx::ansi::font_t::bold | zx::ansi::font_t::italic));
    EXPECT_THAT(
        zx::ansi::font_t::parse("underlined+blink"),
        testing::Optional(zx::ansi::font_t::underlined | zx::ansi::font_t::blink));
    EXPECT_THAT(
        zx::ansi::font_t::parse("italic+underlined+hidden"),
        testing::Optional(zx::ansi::font_t::underlined | zx::ansi::font_t::italic | zx::ansi::font_t::hidden));
}