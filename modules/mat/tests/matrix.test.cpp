#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(matrix, default_construction)
{
    zx::mat::matrix<int, 2, 3> mat;

    EXPECT_THAT(mat, (zx::mat::matrix<int, 2, 3>{ 0, 0, 0, 0, 0, 0 }));
}

TEST(matrix, addition)
{
    zx::mat::matrix<int, 2, 2> a{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2, 2> b{ 5, 6, 7, 8 };

    EXPECT_THAT(a + b, (zx::mat::matrix<int, 2, 2>{ 6, 8, 10, 12 }));
}

TEST(matrix, subtraction)
{
    zx::mat::matrix<int, 2, 2> a{ 5, 7, 9, 11 };
    zx::mat::matrix<int, 2, 2> b{ 1, 2, 3, 4 };

    EXPECT_THAT(a - b, (zx::mat::matrix<int, 2, 2>{ 4, 5, 6, 7 }));
}

TEST(matrix, scalar_multiplication)
{
    zx::mat::matrix<int, 2, 2> a{ 1, 2, 3, 4 };

    EXPECT_THAT(a * 3, (zx::mat::matrix<int, 2, 2>{ 3, 6, 9, 12 }));
    EXPECT_THAT(3 * a, (zx::mat::matrix<int, 2, 2>{ 3, 6, 9, 12 }));
}

TEST(matrix, scalar_division)
{
    zx::mat::matrix<int, 2, 2> a{ 4, 8, 12, 16 };

    EXPECT_THAT(a / 4, (zx::mat::matrix<int, 2, 2>{ 1, 2, 3, 4 }));
}

TEST(matrix, equality)
{
    zx::mat::matrix<int, 2, 2> a{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2, 2> b{ 1, 2, 3, 4 };
    zx::mat::matrix<int, 2, 2> c{ 5, 6, 7, 8 };

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
    zx::mat::matrix<int, 3, 3> mat{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    zx::mat::vector<int, 2> vec{ 1, 2 };

    EXPECT_THAT(mat * vec, (zx::mat::vector<int, 2>{ 16, 20 }));
    EXPECT_THAT(vec * mat, (zx::mat::vector<int, 2>{ 16, 20 }));
}

TEST(matrix, output_stream)
{
    zx::mat::matrix<int, 2, 2> a{ 1, 2, 3, 4 };
    std::ostringstream os;
    os << a;
    EXPECT_THAT(os.str(), "[[1 2][3 4]]");
}
