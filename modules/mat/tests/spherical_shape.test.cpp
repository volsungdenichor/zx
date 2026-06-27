#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(circle, ostream)
{
    zx::mat::spherical_shape_t<2, float> circle{ { 1.0f, 2.0f }, 5.0f };

    std::ostringstream os;
    os << circle;

    EXPECT_THAT(os.str(), "(circle [1 2] 5)");
}

TEST(circle, add_vector)
{
    zx::mat::spherical_shape_t<2, double> circle{ { 1.0, 2.0 }, 3.0 };
    zx::mat::vector_t<2, double> vec{ 10.0, 20.0 };

    zx::mat::spherical_shape_t<2, double> result = circle + vec;

    EXPECT_THAT(result.center, (zx::mat::vector_t<2, double>{ 11.0, 22.0 }));
    EXPECT_THAT(result.radius, 3.0);
}

TEST(circle, subtract_vector)
{
    zx::mat::spherical_shape_t<2, int> circle{ { 10, 20 }, 7 };
    zx::mat::vector_t<2, int> vec{ 1, 2 };

    zx::mat::spherical_shape_t<2, int> result = circle - vec;

    EXPECT_THAT(result.center, (zx::mat::vector_t<2, int>{ 9, 18 }));
    EXPECT_THAT(result.radius, 7);
}

TEST(sphere, ostream)
{
    zx::mat::spherical_shape_t<3, float> sphere{ { 1.0f, 2.0f, 3.0f }, 5.0f };

    std::ostringstream os;
    os << sphere;

    EXPECT_THAT(os.str(), "(sphere [1 2 3] 5)");
}
