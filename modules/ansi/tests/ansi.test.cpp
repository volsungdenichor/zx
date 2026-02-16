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

TEST(ansi, parse_style_info)
{
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("green"),
        testing::Optional(zx::ansi::style_info_t{ zx::ansi::color_t::green, std::nullopt, std::nullopt }));
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("green italic"),
        testing::Optional(zx::ansi::style_info_t{ zx::ansi::color_t::green, std::nullopt, zx::ansi::font_t::italic }));
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("fg:red bg:blue bold"),
        testing::Optional(
            zx::ansi::style_info_t{ zx::ansi::color_t::red, zx::ansi::color_t::blue, zx::ansi::font_t::bold }));
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("bg:yellow underlined"),
        testing::Optional(zx::ansi::style_info_t{ std::nullopt, zx::ansi::color_t::yellow, zx::ansi::font_t::underlined }));
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("fg:0x00FF00 italic"),
        testing::Optional(zx::ansi::style_info_t{ zx::ansi::color_t{ 46 }, std::nullopt, zx::ansi::font_t::italic }));
    EXPECT_THAT(
        zx::ansi::style_info_t::parse("bg:gray:128 underlined"),
        testing::Optional(zx::ansi::style_info_t{ std::nullopt, zx::ansi::color_t{ 244 }, zx::ansi::font_t::underlined }));
    EXPECT_THAT(zx::ansi::style_info_t::parse("invalid"), testing::Eq(std::nullopt));
}

TEST(ansi, stream)
{
    std::stringstream ss;
    zx::ansi::stream_t stream{ std::make_unique<zx::ansi::ostream_stream_t>(ss) };
    stream << "Hello "
           << "world!" << 42 << "." << zx::ansi::styled("fg:red")("This is red text ", 42) << " Normal text.";

    EXPECT_THAT(ss.str(), testing::Eq("Hello world!42.\x1B[38;5;1mThis is red text 42\x1B[0m Normal text."));
}

std::vector<std::string> split_lines(const std::string& text)
{
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
    {
        lines.push_back(line);
    }
    return lines;
}

constexpr auto WhenLinesSplit
    = [](auto&& matcher) { return testing::ResultOf("split lines", split_lines, std::forward<decltype(matcher)>(matcher)); };

TEST(ansi, stream_list)
{
    std::stringstream ss;
    zx::ansi::stream_t stream{ std::make_unique<zx::ansi::ostream_stream_t>(ss) };
    stream << zx::ansi::list("number:1")(
        zx::ansi::list_item("First item"),   //
        zx::ansi::list_item("Second item"),  //
        zx::ansi::list_item("Third item"));

    EXPECT_THAT(
        ss.str(),
        WhenLinesSplit(testing::ElementsAre(
            "1. First item",   //
            "2. Second item",  //
            "3. Third item")));
}

TEST(ansi, stream_nested_list)
{
    std::stringstream ss;
    zx::ansi::stream_t stream{ std::make_unique<zx::ansi::ostream_stream_t>(ss) };
    stream << zx::ansi::list("number:1")(
        zx::ansi::list_item(
            zx::ansi::line("First item"),
            zx::ansi::list("bullet:+")(
                zx::ansi::list_item("uno"),  //
                zx::ansi::list_item("dos"),  //
                zx::ansi::list_item("tres"))),
        zx::ansi::list_item("Second item"),  //
        zx::ansi::list_item("Third item"));

    EXPECT_THAT(
        ss.str(),
        WhenLinesSplit(testing::ElementsAre(
            "1. First item",   //
            "   + uno",        //
            "   + dos",        //
            "   + tres",       //
            "2. Second item",  //
            "3. Third item")));
}

TEST(ansi, map)
{
    std::vector<int> numbers = { 1, 2, 3 };
    std::stringstream ss;
    zx::ansi::stream_t stream{ std::make_unique<zx::ansi::ostream_stream_t>(ss) };
    stream << zx::ansi::list("")(
        zx::ansi::map(numbers, [](int n) -> zx::ansi::node_t { return zx::ansi::span("[", n, "]"); }));
    EXPECT_THAT(ss.str(), testing::Eq("[1] [2] [3]"));
}

TEST(ansi, text_formatting)
{
    std::stringstream ss;
    zx::ansi::stream_t stream{ std::make_unique<zx::ansi::ostream_stream_t>(ss) };
    stream << zx::ansi::format("{1}, {0}!")("world", "Hello");
    EXPECT_THAT(ss.str(), testing::Eq("Hello, world!"));
}
