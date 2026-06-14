#include <gmock/gmock.h>

#include <zx/surface.hpp>

TEST(ansi, surface_render)
{
    auto surface = zx::ansi::surface_t{ zx::ansi::surface_t::size_type{ 2, 2 } };
    surface[{ 0, 0 }] = zx::ansi::cell_t{ zx::code_point_t('A') };
    surface[{ 0, 1 }] = zx::ansi::cell_t{ zx::code_point_t('B') };
    surface[{ 1, 0 }] = zx::ansi::cell_t{ zx::code_point_t('C') };
    surface[{ 1, 1 }] = zx::ansi::cell_t{ zx::code_point_t('D') };

    const std::string expected = "\033[H\033[0m\033[mAB\r\nCD\033[0m";

    EXPECT_THAT(zx::ansi::render(surface), expected);
}

TEST(ansi, surface_render_diff_empty_when_unchanged)
{
    zx::ansi::surface_t prev{ zx::ansi::surface_t::size_type{ 1, 3 } };
    zx::ansi::surface_t next = prev;

    prev[{ 0, 0 }] = zx::ansi::cell_t{ zx::code_point_t('x') };
    prev[{ 0, 1 }] = zx::ansi::cell_t{ zx::code_point_t('y') };
    prev[{ 0, 2 }] = zx::ansi::cell_t{ zx::code_point_t('z') };
    next = prev;

    EXPECT_THAT(zx::ansi::render_diff(prev, next), "");
}

TEST(ansi, surface_render_diff_changed_cell)
{
    zx::ansi::surface_t prev{ zx::ansi::surface_t::size_type{ 1, 3 } };
    zx::ansi::surface_t next{ zx::ansi::surface_t::size_type{ 1, 3 } };

    prev[{ 0, 0 }] = zx::ansi::cell_t{ zx::code_point_t('a') };
    prev[{ 0, 1 }] = zx::ansi::cell_t{ zx::code_point_t('b') };
    prev[{ 0, 2 }] = zx::ansi::cell_t{ zx::code_point_t('c') };

    next = prev;
    next[{ 0, 1 }] = zx::ansi::cell_t{ zx::code_point_t('X') };

    const std::string expected = "\033[1;2H\033[0m\033[mX\033[0m";

    EXPECT_THAT(zx::ansi::render_diff(prev, next), expected);
}

TEST(ansi, surface_render_large_with_styles_and_unicode)
{
    const zx::ansi::style_info_t red_bold = { zx::ansi::color_t::red, {}, zx::ansi::font_t::bold };
    const zx::ansi::style_info_t green_bg_underlined = { {}, zx::ansi::color_t::green, zx::ansi::font_t::underlined };
    const zx::ansi::style_info_t blue_italic = { zx::ansi::color_t::blue, {}, zx::ansi::font_t::italic };

    auto surface = zx::ansi::surface_t{ zx::ansi::surface_t::size_type{ 2, 5 } };

    surface[{ 0, 0 }] = zx::ansi::cell_t{ zx::code_point_t('H') };
    surface[{ 0, 1 }] = zx::ansi::cell_t{ zx::code_point_t('e'), red_bold };
    surface[{ 0, 2 }] = zx::ansi::cell_t{ zx::code_point_t("ł"), red_bold };
    surface[{ 0, 3 }] = zx::ansi::cell_t{ zx::code_point_t("ł"), green_bg_underlined };
    surface[{ 0, 4 }] = zx::ansi::cell_t{ zx::code_point_t('o'), green_bg_underlined };

    surface[{ 1, 0 }] = zx::ansi::cell_t{ zx::code_point_t(' ') };
    surface[{ 1, 1 }] = zx::ansi::cell_t{ zx::code_point_t("🌍"), blue_italic };
    surface[{ 1, 2 }] = zx::ansi::cell_t{ zx::code_point_t("中"), blue_italic };
    surface[{ 1, 3 }] = zx::ansi::cell_t{ zx::code_point_t('!') };
    surface[{ 1, 4 }] = zx::ansi::cell_t{ zx::code_point_t('!') };

    const std::string expected
        = "\x1B[H\x1B[0m\x1B[mH\x1B[0m\x1B[38;5;1;1me\xC5\x82\x1B[0m\x1B[48;5;2;4m\xC5\x82o\r\n\x1B[0m\x1B[m "
          "\x1B[0m\x1B[38;5;4;3m\xF0\x9F\x8C\x8D\xE4\xB8\xAD\x1B[0m\x1B[m!!\x1B[0m";

    EXPECT_THAT(zx::ansi::render(surface), expected);
}

TEST(ansi, surface_render_diff_large_with_styles_and_unicode)
{
    const zx::ansi::style_info_t red_bold = { zx::ansi::color_t::red, {}, zx::ansi::font_t::bold };
    const zx::ansi::style_info_t blue_italic = { zx::ansi::color_t::blue, {}, zx::ansi::font_t::italic };

    const auto prev = zx::ansi::surface_t{ zx::ansi::surface_t::size_type{ 3, 5 }, zx::code_point_t('a') };

    zx::ansi::surface_t next = prev;
    next[{ 0, 2 }] = zx::ansi::cell_t{ zx::code_point_t("Ω"), red_bold };
    next[{ 1, 0 }] = zx::ansi::cell_t{ zx::code_point_t("中") };
    next[{ 2, 3 }] = zx::ansi::cell_t{ zx::code_point_t("ß"), blue_italic };
    next[{ 2, 4 }] = zx::ansi::cell_t{ zx::code_point_t("🚀"), blue_italic };

    const std::string expected
        = "\x1B[1;3H\x1B[0m\x1B[38;5;1;1m\xCE\xA9\x1B[2;1H\x1B[0m\x1B[m\xE4\xB8\xAD\x1B[3;4H\x1B[0m\x1B[38;5;4;"
          "3m\xC3\x9F\xF0\x9F\x9A\x80\x1B[0m";

    EXPECT_THAT(zx::ansi::render_diff(prev, next), expected);
}
