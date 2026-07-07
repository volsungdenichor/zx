#include <gmock/gmock.h>

#include <zx/mat.hpp>

#include "matchers.hpp"

TEST(mat, dot_product)
{
    zx::mat::vector_t<3, double> v1{ 1.0, 2.0, 3.0 };
    zx::mat::vector_t<3, double> v2{ 4.0, 5.0, 6.0 };

    EXPECT_THAT(zx::mat::dot(v1, v2), ApproxEqual(32.0));
}

TEST(mat, altitude)
{
    zx::mat::triangle_t<2, double> t{ zx::mat::vector_t<2, double>{ 0.0, 0.0 },
                                      zx::mat::vector_t<2, double>{ 4.0, 0.0 },
                                      zx::mat::vector_t<2, double>{ 0.0, 4.0 } };

    EXPECT_THAT(
        zx::mat::altitude(t, 0),
        ApproxEqual(zx::mat::segment_t<2, double>{ zx::mat::vector_t<2, double>{ 0.0, 0.0 },
                                                   zx::mat::vector_t<2, double>{ 2.0, 2.0 } }));
}

TEST(mat, interval_intersection_overlapping_intervals)
{
    const zx::mat::interval_t<int> a{ 1, 4 };
    const zx::mat::interval_t<int> b{ 3, 7 };

    EXPECT_THAT(zx::mat::intersects(a, b), testing::IsTrue());
    EXPECT_THAT(zx::mat::intersects(b, a), testing::IsTrue());
}

TEST(mat, interval_intersection_touching_endpoints_do_not_intersect)
{
    const zx::mat::interval_t<int> a{ 1, 3 };
    const zx::mat::interval_t<int> b{ 3, 8 };

    EXPECT_THAT(zx::mat::intersects(a, b), testing::IsFalse());
    EXPECT_THAT(zx::mat::intersects(b, a), testing::IsFalse());
}

TEST(mat, interval_intersection_disjoint_intervals)
{
    const zx::mat::interval_t<int> a{ 1, 2 };
    const zx::mat::interval_t<int> b{ 5, 9 };

    EXPECT_THAT(zx::mat::intersects(a, b), testing::IsFalse());
    EXPECT_THAT(zx::mat::intersects(b, a), testing::IsFalse());
}
