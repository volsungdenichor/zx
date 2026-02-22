#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(circle, ostream)
{
    zx::mat::spherical_shape_t<float, 2> circle{ { 1.0f, 2.0f }, 5.0f };

    std::ostringstream os;
    os << circle;

    EXPECT_THAT(os.str(), "(circle [1 2] 5)");
}

TEST(circle, add_vector)
{
    zx::mat::spherical_shape_t<double, 2> circle{ { 1.0, 2.0 }, 3.0 };
    zx::mat::vector<double, 2> vec{ 10.0, 20.0 };

    zx::mat::spherical_shape_t<double, 2> result = circle + vec;

    EXPECT_THAT(result.center, (zx::mat::vector<double, 2>{ 11.0, 22.0 }));
    EXPECT_THAT(result.radius, 3.0);
}

TEST(circle, subtract_vector)
{
    zx::mat::spherical_shape_t<int, 2> circle{ { 10, 20 }, 7 };
    zx::mat::vector<int, 2> vec{ 1, 2 };

    zx::mat::spherical_shape_t<int, 2> result = circle - vec;

    EXPECT_THAT(result.center, (zx::mat::vector<int, 2>{ 9, 18 }));
    EXPECT_THAT(result.radius, 7);
}

TEST(sphere, ostream)
{
    zx::mat::spherical_shape_t<float, 3> sphere{ { 1.0f, 2.0f, 3.0f }, 5.0f };

    std::ostringstream os;
    os << sphere;

    EXPECT_THAT(os.str(), "(sphere [1 2 3] 5)");
}
