#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(circle, ostream)
{
    zx::mat::spherical_shape_t<2, float> circle{ { 1.0f, 2.0f }, 5.0f };

    std::ostringstream os;
    os << circle;

    EXPECT_THAT(os.str(), "(circle [1 2] 5)");
}

TEST(circle, translate)
{
    EXPECT_THAT(
        zx::mat::translate(
            zx::mat::spherical_shape_t<2, double>{ { 1.0, 2.0 }, 3.0 }, zx::mat::vector_t<2, double>{ 10.0, 20.0 }),
        testing::AllOf(
            testing::ResultOf("center", zx::mat::center, (zx::mat::vector_t<2, double>{ 11.0, 22.0 })),
            testing::ResultOf("radius", zx::mat::radius, 3.0)));

    EXPECT_THAT(
        zx::mat::translate(zx::mat::spherical_shape_t<2, int>{ { 10, 20 }, 7 }, zx::mat::vector_t<2, int>{ -1, -2 }),
        testing::AllOf(
            testing::ResultOf("center", zx::mat::center, (zx::mat::vector_t<2, int>{ 9, 18 })),
            testing::ResultOf("radius", zx::mat::radius, 7)));
}

TEST(sphere, ostream)
{
    zx::mat::spherical_shape_t<3, float> sphere{ { 1.0f, 2.0f, 3.0f }, 5.0f };

    std::ostringstream os;
    os << sphere;

    EXPECT_THAT(os.str(), "(sphere [1 2 3] 5)");
}
