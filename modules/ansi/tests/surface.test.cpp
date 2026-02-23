#include <gmock/gmock.h>

#include <zx/surface.hpp>

TEST(ansi, surface_render)
{
    zx::ansi::surface_t surface{ zx::ansi::surface_t::size_type{ 2, 3 } };
    surface[{ 0, 0 }] = zx::glyph_t('a');
    surface[{ 0, 1 }] = zx::glyph_t("❤");
    surface[{ 0, 2 }] = zx::glyph_t('c');
    surface[{ 1, 0 }] = zx::glyph_t('d');
    surface[{ 1, 1 }] = zx::glyph_t("🔴");
    surface[{ 1, 2 }] = zx::glyph_t('f');

    EXPECT_THAT(zx::ansi::render_lines(surface), testing::ElementsAre("a❤c", "d🔴f"));
}
