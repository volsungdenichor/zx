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
