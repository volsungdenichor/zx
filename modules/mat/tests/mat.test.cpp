#include <gmock/gmock.h>

#include <zx/mat.hpp>

#include "matchers.hpp"

TEST(mat, dot_product)
{
    const auto v1 = zx::mat::vector(1.0, 2.0, 3.0);
    const auto v2 = zx::mat::vector(4.0, 5.0, 6.0);

    EXPECT_THAT(zx::mat::dot(v1, v2), ApproxEqual(32.0));
}

TEST(mat, altitude)
{
    const auto t = zx::mat::triangle(zx::mat::vector(0.0, 0.0), zx::mat::vector(4.0, 0.0), zx::mat::vector(0.0, 4.0));

    EXPECT_THAT(
        zx::mat::altitude(t, 0),
        ApproxEqual(zx::mat::segment_t<2, double>{ zx::mat::vector(0.0, 0.0), zx::mat::vector(2.0, 2.0) }));
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

TEST(mat, interval_union_of_many)
{
    const zx::mat::interval_t<int> a{ 3, 8 };
    const zx::mat::interval_t<int> b{ 1, 4 };
    const zx::mat::interval_t<int> c{ 9, 12 };
    const zx::mat::interval_t<int> d{ 2, 10 };

    EXPECT_THAT(zx::mat::unite(a, b, c, d), testing::Eq((zx::mat::interval_t<int>{ 1, 12 })));
}

TEST(mat, interval_intersection_of_many)
{
    const zx::mat::interval_t<int> a{ 1, 10 };
    const zx::mat::interval_t<int> b{ 2, 9 };
    const zx::mat::interval_t<int> c{ 3, 8 };

    EXPECT_THAT(zx::mat::intersection(a, b, c), testing::Optional(testing::Eq((zx::mat::interval_t<int>{ 3, 8 }))));
}

TEST(mat, interval_intersection_of_many_returns_nullopt_when_empty)
{
    const zx::mat::interval_t<int> a{ 1, 8 };
    const zx::mat::interval_t<int> b{ 2, 7 };
    const zx::mat::interval_t<int> c{ 7, 9 };

    EXPECT_THAT(zx::mat::intersection(a, b, c), testing::Eq(std::nullopt));
}

TEST(mat, box_union_of_many)
{
    const zx::mat::box_shape_t<2, int> a{ zx::mat::interval_t<int>{ 1, 4 }, zx::mat::interval_t<int>{ 10, 20 } };
    const zx::mat::box_shape_t<2, int> b{ zx::mat::interval_t<int>{ 0, 2 }, zx::mat::interval_t<int>{ 30, 40 } };
    const zx::mat::box_shape_t<2, int> c{ zx::mat::interval_t<int>{ 2, 8 }, zx::mat::interval_t<int>{ 15, 35 } };

    EXPECT_THAT(
        zx::mat::unite(a, b, c),
        testing::Eq((zx::mat::box_shape_t<2, int>{ zx::mat::interval_t<int>{ 0, 8 }, zx::mat::interval_t<int>{ 10, 40 } })));
}

TEST(mat, box_intersection_of_many)
{
    const zx::mat::box_shape_t<2, int> a{ zx::mat::interval_t<int>{ 0, 10 }, zx::mat::interval_t<int>{ 0, 10 } };
    const zx::mat::box_shape_t<2, int> b{ zx::mat::interval_t<int>{ 2, 8 }, zx::mat::interval_t<int>{ 3, 9 } };
    const zx::mat::box_shape_t<2, int> c{ zx::mat::interval_t<int>{ 4, 7 }, zx::mat::interval_t<int>{ 1, 6 } };

    EXPECT_THAT(
        zx::mat::intersection(a, b, c),
        testing::Optional(testing::Eq(
            (zx::mat::box_shape_t<2, int>{ zx::mat::interval_t<int>{ 4, 7 }, zx::mat::interval_t<int>{ 3, 6 } }))));
}

TEST(mat, box_intersection_of_many_returns_nullopt_when_empty)
{
    const zx::mat::box_shape_t<2, int> a{ zx::mat::interval_t<int>{ 0, 4 }, zx::mat::interval_t<int>{ 0, 4 } };
    const zx::mat::box_shape_t<2, int> b{ zx::mat::interval_t<int>{ 2, 8 }, zx::mat::interval_t<int>{ 1, 3 } };
    const zx::mat::box_shape_t<2, int> c{ zx::mat::interval_t<int>{ 4, 9 }, zx::mat::interval_t<int>{ 1, 2 } };

    EXPECT_THAT(zx::mat::intersection(a, b, c), testing::Eq(std::nullopt));
}

TEST(mat, segments)
{
    EXPECT_THAT(
        zx::mat::segments(
            zx::mat::quad(zx::mat::point(0, 0), zx::mat::point(1, 0), zx::mat::point(1, 1), zx::mat::point(0, 1))),
        testing::ElementsAre(
            zx::mat::segment(zx::mat::point(0, 0), zx::mat::point(1, 0)),
            zx::mat::segment(zx::mat::point(1, 0), zx::mat::point(1, 1)),
            zx::mat::segment(zx::mat::point(1, 1), zx::mat::point(0, 1)),
            zx::mat::segment(zx::mat::point(0, 1), zx::mat::point(0, 0))));

    EXPECT_THAT(
        zx::mat::segments(zx::mat::triangle(zx::mat::point(0, 0), zx::mat::point(1, 0), zx::mat::point(1, 1))),
        testing::ElementsAre(
            zx::mat::segment(zx::mat::point(0, 0), zx::mat::point(1, 0)),
            zx::mat::segment(zx::mat::point(1, 0), zx::mat::point(1, 1)),
            zx::mat::segment(zx::mat::point(1, 1), zx::mat::point(0, 0))));

    EXPECT_THAT(
        zx::mat::segments(
            zx::mat::polygon(zx::mat::point(0, 0), zx::mat::point(1, 0), zx::mat::point(1, 1), zx::mat::point(0, 1))),
        testing::ElementsAre(
            zx::mat::segment(zx::mat::point(0, 0), zx::mat::point(1, 0)),
            zx::mat::segment(zx::mat::point(1, 0), zx::mat::point(1, 1)),
            zx::mat::segment(zx::mat::point(1, 1), zx::mat::point(0, 1)),
            zx::mat::segment(zx::mat::point(0, 1), zx::mat::point(0, 0))));

    EXPECT_THAT(
        zx::mat::segments(
            zx::mat::polyline(zx::mat::point(0, 0), zx::mat::point(1, 0), zx::mat::point(1, 1), zx::mat::point(0, 1))),
        testing::ElementsAre(
            zx::mat::segment(zx::mat::point(0, 0), zx::mat::point(1, 0)),
            zx::mat::segment(zx::mat::point(1, 0), zx::mat::point(1, 1)),
            zx::mat::segment(zx::mat::point(1, 1), zx::mat::point(0, 1))));

    EXPECT_THAT(
        zx::mat::segments(zx::mat::box::from_lower_upper(zx::mat::point(0, 0), zx::mat::point(2, 2))),
        testing::ElementsAre(
            zx::mat::segment(zx::mat::point(0, 0), zx::mat::point(1, 0)),
            zx::mat::segment(zx::mat::point(1, 0), zx::mat::point(1, 1)),
            zx::mat::segment(zx::mat::point(1, 1), zx::mat::point(0, 1)),
            zx::mat::segment(zx::mat::point(0, 1), zx::mat::point(0, 0))));
}