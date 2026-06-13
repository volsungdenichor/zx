#include <gmock/gmock.h>

#include <zx/surface.hpp>

TEST(ansi, surface_render)
{
    zx::ansi::surface_t surface{ zx::ansi::surface_t::size_type{ 2, 3 } };
    surface[{ 0, 0 }] = zx::string_t::value_type('a');
    surface[{ 0, 1 }] = zx::string_t::value_type("❤");
    surface[{ 0, 2 }] = zx::string_t::value_type('c');
    surface[{ 1, 0 }] = zx::string_t::value_type('d');
    surface[{ 1, 1 }] = zx::string_t::value_type("🔴");
    surface[{ 1, 2 }] = zx::string_t::value_type('f');

    EXPECT_THAT(zx::ansi::render_lines(surface), testing::ElementsAre("a❤c", "d🔴f"));
}
