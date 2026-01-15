#include <gmock/gmock.h>

#include <zx/mat.hpp>

#include "matchers.hpp"

TEST(matrix, default_construction)
{
    zx::mat::matrix<int, 2, 3> mat;

    EXPECT_THAT(mat, (zx::mat::matrix<int, 2, 3>{ 0, 0, 0, 0, 0, 0 }));
}

TEST(matrix, addition)
{
    zx::mat::matrix<int, 2> a{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2> b{ 5, 6, 7, 8 };

    EXPECT_THAT(a + b, (zx::mat::matrix<int, 2, 2>{ 6, 8, 10, 12 }));
}

TEST(matrix, subtraction)
{
    zx::mat::matrix<int, 2> a{ 5, 7, 9, 11 };
    zx::mat::matrix<int, 2> b{ 1, 2, 3, 4 };

    EXPECT_THAT(a - b, (zx::mat::matrix<int, 2, 2>{ 4, 5, 6, 7 }));
}

TEST(matrix, scalar_multiplication)
{
    zx::mat::matrix<int, 2> a{ 1, 2, 3, 4 };

    EXPECT_THAT(a * 3, (zx::mat::matrix<int, 2>{ 3, 6, 9, 12 }));
    EXPECT_THAT(3 * a, (zx::mat::matrix<int, 2>{ 3, 6, 9, 12 }));
}

TEST(matrix, scalar_division)
{
    zx::mat::matrix<int, 2> a{ 4, 8, 12, 16 };

    EXPECT_THAT(a / 4, (zx::mat::matrix<int, 2>{ 1, 2, 3, 4 }));
}

TEST(matrix, equality)
{
    zx::mat::matrix<int, 2> a{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2> b{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2> c{ 5, 6, 7, 8 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(matrix, multiplication)
{
    zx::mat::matrix<int, 2, 3> a{ 1, 2, 3, 4, 5, 6 };
    zx::mat::matrix<int, 3, 2> b{ 7, 8, 9, 10, 11, 12 };

    EXPECT_THAT(a * b, (zx::mat::matrix<int, 2, 2>{ 58, 64, 139, 154 }));
}

TEST(matrix, multiplication_with_vector)
{
    zx::mat::matrix<int, 3> mat{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    zx::mat::vector<int, 2> vec{ 1, 2 };

    EXPECT_THAT(mat * vec, (zx::mat::vector<int, 2>{ 16, 20 }));
    EXPECT_THAT(vec * mat, (zx::mat::vector<int, 2>{ 16, 20 }));
}

TEST(matrix, output_stream)
{
    zx::mat::matrix<int, 2> a{ 1, 2, 3, 4 };
    std::ostringstream os;
    os << a;
    EXPECT_THAT(os.str(), "[[1 2] [3 4]]");
}

TEST(matrix, transpose)
{
    zx::mat::matrix<int, 2, 3> a{ 1, 2, 3, 4, 5, 6 };
    EXPECT_THAT(zx::mat::transpose(a), (zx::mat::matrix<int, 3, 2>{ 1, 4, 2, 5, 3, 6 }));
}

TEST(matrix, minor)
{
    zx::mat::matrix<int, 3> a{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    EXPECT_THAT(zx::mat::minor(a, { 0, 0 }), (zx::mat::matrix<int, 2>{ 5, 6, 8, 9 }));
    EXPECT_THAT(zx::mat::minor(a, { 1, 1 }), (zx::mat::matrix<int, 2>{ 1, 3, 7, 9 }));
    EXPECT_THAT(zx::mat::minor(a, { 2, 2 }), (zx::mat::matrix<int, 2>{ 1, 2, 4, 5 }));
}

TEST(matrix, determinant)
{
    zx::mat::matrix<double, 2> a{ 1.0, 2.0, 3.0, 4.0 };
    EXPECT_DOUBLE_EQ(zx::mat::determinant(a), -2.0);

    zx::mat::matrix<double, 3> b{ 6.0, 1.0, 1.0, 4.0, -2.0, 5.0, 2.0, 8.0, 7.0 };
    EXPECT_DOUBLE_EQ(zx::mat::determinant(b), -306.0);
}

TEST(matrix, invert)
{
    zx::mat::matrix<double, 2> a{ 4.0, 7.0, 2.0, 6.0 };
    EXPECT_THAT(zx::mat::invert(a), (zx::mat::matrix<double, 2>{ 0.6, -0.7, -0.2, 0.4 }));

    zx::mat::matrix<double, 3> b{ 1.0, 2.0, 3.0, 0.0, 1.0, 4.0, 5.0, 6.0, 0.0 };
    EXPECT_THAT(
        zx::mat::invert(b), (zx::mat::matrix<double, 3>{ -24.0, 18.0, 5.0, 20.0, -15.0, -4.0, -5.0, 4.0, 1.0 } / 1.0));
}

TEST(matrix, scale)
{
    zx::mat::matrix<double, 3> scale2d = zx::mat::scale(zx::mat::vector<double, 2>{ 2.0, 3.0 });
    EXPECT_THAT(scale2d, (zx::mat::matrix<double, 3>{ 2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 1.0 }));

    zx::mat::matrix<double, 4> scale3d = zx::mat::scale(zx::mat::vector<double, 3>{ 2.0, 3.0, 4.0 });
    EXPECT_THAT(
        scale3d,
        (zx::mat::matrix<double, 4>{ 2.0, 0.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0 }));
}

TEST(matrix, translation)
{
    zx::mat::matrix<double, 3> translation2d = zx::mat::translation(zx::mat::vector<double, 2>{ 5.0, 7.0 });
    EXPECT_THAT(translation2d, (zx::mat::matrix<double, 3>{ 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 7.0, 1.0 }));

    zx::mat::matrix<double, 4> translation3d = zx::mat::translation(zx::mat::vector<double, 3>{ 5.0, 7.0, 9.0 });
    EXPECT_THAT(
        translation3d,
        (zx::mat::matrix<double, 4>{ 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 7.0, 9.0, 1.0 }));
}

TEST(matrix, rotation)
{
    zx::mat::matrix<double, 3> rotation2d = zx::mat::rotation(zx::mat::math::pi<double> / 2.0);
    EXPECT_THAT(rotation2d, ApproxEqual(zx::mat::matrix<double, 3>{ 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0 }));
}
