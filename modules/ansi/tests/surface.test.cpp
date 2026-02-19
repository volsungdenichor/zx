#include <gmock/gmock.h>

#include <zx/surface.hpp>

TEST(ansi, surface_render)
{
    zx::ansi::surface_t surface{ zx::ansi::extent_t{ 3, 2 } };
    surface[{ 0, 0 }] = 'a';
    surface[{ 1, 0 }] = 'b';
    surface[{ 2, 0 }] = 'c';
    surface[{ 0, 1 }] = 'd';
    surface[{ 1, 1 }] = 'e';
    surface[{ 2, 1 }] = 'f';

    EXPECT_THAT(zx::ansi::render_lines(surface), testing::ElementsAre("abc", "def"));
}