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
    zx::ansi::string_t str = "❤Hello, world!🔴";
    EXPECT_THAT(str, testing::ElementsAre(U'❤', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', "🔴"));
}
