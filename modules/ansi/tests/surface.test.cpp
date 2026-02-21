#include <gmock/gmock.h>

#include <zx/surface.hpp>

TEST(ansi, surface_render)
{
    zx::ansi::surface_t surface{ zx::ansi::extent_t{ 3, 2 } };
    surface[{ 0, 0 }] = zx::ansi::glyph_t('a');
    surface[{ 1, 0 }] = zx::ansi::glyph_t("❤");
    surface[{ 2, 0 }] = zx::ansi::glyph_t('c');
    surface[{ 0, 1 }] = zx::ansi::glyph_t('d');
    surface[{ 1, 1 }] = zx::ansi::glyph_t("🔴");
    surface[{ 2, 1 }] = zx::ansi::glyph_t('f');

    EXPECT_THAT(zx::ansi::render_lines(surface), testing::ElementsAre("a❤c", "d🔴f"));
}

TEST(ansi, string)
{
    using g = zx::ansi::string_t::value_type;
    EXPECT_THAT(
        zx::ansi::string_t{ "❤Hello, world!🔴" },
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
