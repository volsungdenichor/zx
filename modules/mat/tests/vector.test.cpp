#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(vector_t, default_construction)
{
    zx::mat::vector_t<3, int> vec;

    EXPECT_THAT(vec, (zx::mat::vector_t<3, int>{ 0, 0, 0 }));
}

TEST(vector_t, addition)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };
    zx::mat::vector_t<3, int> b{ 4, 5, 6 };

    EXPECT_THAT(a + b, (zx::mat::vector_t<3, int>{ 5, 7, 9 }));
}

TEST(vector_t, output_stream)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };
    std::ostringstream os;
    os << a;
    EXPECT_THAT(os.str(), "[1 2 3]");
}

TEST(vector_t, subtraction)
{
    zx::mat::vector_t<3, int> a{ 5, 7, 9 };
    zx::mat::vector_t<3, int> b{ 4, 5, 6 };

    EXPECT_THAT(a - b, (zx::mat::vector_t<3, int>{ 1, 2, 3 }));
}

TEST(vector_t, scalar_multiplication)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };

    EXPECT_THAT(a * 3, (zx::mat::vector_t<3, int>{ 3, 6, 9 }));
    EXPECT_THAT(3 * a, (zx::mat::vector_t<3, int>{ 3, 6, 9 }));
}

TEST(vector_t, scalar_division)
{
    zx::mat::vector_t<3, int> a{ 3, 6, 9 };

    EXPECT_THAT(a / 3, (zx::mat::vector_t<3, int>{ 1, 2, 3 }));
}

TEST(vector_t, equality)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };
    zx::mat::vector_t<3, int> b{ 1, 2, 3 };
    zx::mat::vector_t<3, int> c{ 4, 5, 6 };

    EXPECT_THAT(a == b, testing::IsTrue());
    EXPECT_THAT(a == c, testing::IsFalse());
    EXPECT_THAT(a != c, testing::IsTrue());
    EXPECT_THAT(a != b, testing::IsFalse());
}

TEST(vector_t, negation)
{
    zx::mat::vector_t<3, int> a{ 1, -2, 3 };

    EXPECT_THAT(-a, (zx::mat::vector_t<3, int>{ -1, 2, -3 }));
}

TEST(vector_t, addition_of_mixed_types)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };
    zx::mat::vector_t<3, double> b{ 4.5, 5.5, 6.5 };

    EXPECT_THAT(a + b, (zx::mat::vector_t<3, double>{ 5.5, 7.5, 9.5 }));
}

TEST(vector_t, subtraction_of_mixed_types)
{
    zx::mat::vector_t<3, int> a{ 5, 7, 9 };
    zx::mat::vector_t<3, double> b{ 4.5, 5.5, 6.5 };

    EXPECT_THAT(a - b, (zx::mat::vector_t<3, double>{ 0.5, 1.5, 2.5 }));
}

TEST(vector_t, scalar_multiplication_of_mixed_types)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };

    EXPECT_THAT(a * 2.5, (zx::mat::vector_t<3, double>{ 2.5, 5.0, 7.5 }));
    EXPECT_THAT(2.5 * a, (zx::mat::vector_t<3, double>{ 2.5, 5.0, 7.5 }));
}

TEST(vector_t, scalar_division_of_mixed_types)
{
    zx::mat::vector_t<3, double> a{ 2.5, 5.0, 7.5 };

    EXPECT_THAT(a / 2, (zx::mat::vector_t<3, double>{ 1.25, 2.5, 3.75 }));
}

TEST(vector_t, equality_of_mixed_types)
{
    zx::mat::vector_t<3, int> a{ 1, 2, 3 };
    zx::mat::vector_t<3, double> b{ 1.0, 2.0, 3.0 };
    zx::mat::vector_t<3, double> c{ 4.0, 5.0, 6.0 };

    EXPECT_THAT(a == b, testing::IsTrue());
    EXPECT_THAT(a == c, testing::IsFalse());
    EXPECT_THAT(a != c, testing::IsTrue());
    EXPECT_THAT(a != b, testing::IsFalse());
}
