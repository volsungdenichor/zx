#include <gmock/gmock.h>

#include <zx/mat.hpp>

#include "matchers.hpp"

TEST(mat, dot_product)
{
    zx::mat::vector<double, 3> v1{ 1.0, 2.0, 3.0 };
    zx::mat::vector<double, 3> v2{ 4.0, 5.0, 6.0 };

    EXPECT_THAT(zx::mat::dot(v1, v2), ApproxEqual(32.0));
}

TEST(mat, altitude)
{
    zx::mat::triangle_t<double, 2> t{ zx::mat::vector<double, 2>{ 0.0, 0.0 },
                                      zx::mat::vector<double, 2>{ 4.0, 0.0 },
                                      zx::mat::vector<double, 2>{ 0.0, 4.0 } };

    EXPECT_THAT(
        zx::mat::altitude(t, 0),
        ApproxEqual(zx::mat::segment_t<double, 2>{ zx::mat::vector<double, 2>{ 0.0, 0.0 },
                                                   zx::mat::vector<double, 2>{ 2.0, 2.0 } }));
}
