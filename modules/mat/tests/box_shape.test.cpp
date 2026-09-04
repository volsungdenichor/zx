#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(box, default_construction)
{
    zx::mat::box_shape_t<3, int> box;

    EXPECT_THAT(
        box,
        (zx::mat::box_shape_t<3, int>{
            zx::mat::interval_t<int>{ 0, 0 }, zx::mat::interval_t<int>{ 0, 0 }, zx::mat::interval_t<int>{ 0, 0 } }));
}

TEST(box, parameterized_construction)
{
    zx::mat::rectangle_t<int> box{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } };

    EXPECT_THAT(box, (zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } }));
}

TEST(box, construction_from_center_and_asymmetric_radius)
{
    EXPECT_THAT(
        zx::mat::box::from_center_radius(
            zx::mat::point(10, 20), zx::mat::vector(2, 3), zx::mat::vector(4, 5)),
        (zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 8, 15 }, zx::mat::interval_t<int>{ 17, 26 } }));
}

TEST(box, construction_from_center_and_radius)
{
    EXPECT_THAT(
        zx::mat::box::from_center_radius(zx::mat::point(10, 20), zx::mat::vector(2, 3)),
        (zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 8, 13 }, zx::mat::interval_t<int>{ 17, 24 } }));

    EXPECT_THAT(
        zx::mat::box::from_center_radius(zx::mat::point(10.0, 20.0), zx::mat::vector(2.0, 3.0)),
        (zx::mat::rectangle_t<double>{
            zx::mat::interval_t<double>{ 8.0, 12.0 }, zx::mat::interval_t<double>{ 17.0, 23.0 } }));
}

TEST(box, translation)
{
    EXPECT_THAT(
        zx::mat::translate(
            zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } },
            zx::mat::vector_t<2, int>{ 3, 14 }),
        (zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 4, 8 }, zx::mat::interval_t<int>{ 24, 34 } }));

    EXPECT_THAT(
        zx::mat::translate(
            zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 5, 10 }, zx::mat::interval_t<int>{ 20, 30 } },
            zx::mat::vector_t<2, int>{ -2, -15 }),
        (zx::mat::rectangle_t<int>{ zx::mat::interval_t<int>{ 3, 8 }, zx::mat::interval_t<int>{ 5, 15 } }));
}

TEST(box, equality)
{
    zx::mat::rectangle_t<int> a{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } };
    zx::mat::rectangle_t<int> b{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } };
    zx::mat::rectangle_t<int> c{ zx::mat::interval_t<int>{ 2, 6 }, zx::mat::interval_t<int>{ 15, 25 } };

    EXPECT_THAT(a == b, testing::IsTrue());
    EXPECT_THAT(a == c, testing::IsFalse());
    EXPECT_THAT(a != c, testing::IsTrue());
    EXPECT_THAT(a != b, testing::IsFalse());
}

TEST(box, output_stream)
{
    zx::mat::rectangle_t<int> box{ zx::mat::interval_t<int>{ 1, 5 }, zx::mat::interval_t<int>{ 10, 20 } };
    std::ostringstream os;
    os << box;
    EXPECT_THAT(os.str(), "[[1 5) [10 20)]");
}
