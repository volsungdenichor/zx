#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(mat, dot_product)
{
    zx::mat::vector<double, 3> v1{ 1.0, 2.0, 3.0 };
    zx::mat::vector<double, 3> v2{ 4.0, 5.0, 6.0 };

    EXPECT_DOUBLE_EQ(zx::mat::dot(v1, v2), 32.0);
}

TEST(mat, altitude)
{
    zx::mat::triangle<double, 2> tri{ zx::mat::vector<double, 2>{ 0.0, 0.0 },
                                      zx::mat::vector<double, 2>{ 4.0, 0.0 },
                                      zx::mat::vector<double, 2>{ 0.0, 4.0 } };

    EXPECT_THAT(
        zx::mat::altitude(tri, 0),
        testing::ElementsAre((zx::mat::vector<double, 2>{ 0.0, 0.0 }), (zx::mat::vector<double, 2>{ 2.0, 2.0 })));
}
