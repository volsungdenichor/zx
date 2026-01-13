#include <gmock/gmock.h>

#include <zx/vector.hpp>

TEST(vector, default_construction)
{
    zx::mat::vector<int, 3> vec;

    EXPECT_THAT(vec, (zx::mat::vector<int, 3>{ 0, 0, 0 }));
}

TEST(vector, addition)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };
    zx::mat::vector<int, 3> b{ 4, 5, 6 };

    EXPECT_THAT(a + b, (zx::mat::vector<int, 3>{ 5, 7, 9 }));
}

TEST(vector, output_stream)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };
    std::ostringstream os;
    os << a;
    EXPECT_THAT(os.str(), "[1 2 3]");
}

TEST(vector, subtraction)
{
    zx::mat::vector<int, 3> a{ 5, 7, 9 };
    zx::mat::vector<int, 3> b{ 4, 5, 6 };

    EXPECT_THAT(a - b, (zx::mat::vector<int, 3>{ 1, 2, 3 }));
}

TEST(vector, scalar_multiplication)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };

    EXPECT_THAT(a * 3, (zx::mat::vector<int, 3>{ 3, 6, 9 }));
    EXPECT_THAT(3 * a, (zx::mat::vector<int, 3>{ 3, 6, 9 }));
}

TEST(vector, scalar_division)
{
    zx::mat::vector<int, 3> a{ 3, 6, 9 };

    EXPECT_THAT(a / 3, (zx::mat::vector<int, 3>{ 1, 2, 3 }));
}

TEST(vector, equality)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };
    zx::mat::vector<int, 3> b{ 1, 2, 3 };
    zx::mat::vector<int, 3> c{ 4, 5, 6 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(vector, negation)
{
    zx::mat::vector<int, 3> a{ 1, -2, 3 };

    EXPECT_THAT(-a, (zx::mat::vector<int, 3>{ -1, 2, -3 }));
}

TEST(vector, addition_of_mixed_types)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };
    zx::mat::vector<double, 3> b{ 4.5, 5.5, 6.5 };

    EXPECT_THAT(a + b, (zx::mat::vector<double, 3>{ 5.5, 7.5, 9.5 }));
}

TEST(vector, subtraction_of_mixed_types)
{
    zx::mat::vector<int, 3> a{ 5, 7, 9 };
    zx::mat::vector<double, 3> b{ 4.5, 5.5, 6.5 };

    EXPECT_THAT(a - b, (zx::mat::vector<double, 3>{ 0.5, 1.5, 2.5 }));
}

TEST(vector, scalar_multiplication_of_mixed_types)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };

    EXPECT_THAT(a * 2.5, (zx::mat::vector<double, 3>{ 2.5, 5.0, 7.5 }));
    EXPECT_THAT(2.5 * a, (zx::mat::vector<double, 3>{ 2.5, 5.0, 7.5 }));
}

TEST(vector, scalar_division_of_mixed_types)
{
    zx::mat::vector<double, 3> a{ 2.5, 5.0, 7.5 };

    EXPECT_THAT(a / 2, (zx::mat::vector<double, 3>{ 1.25, 2.5, 3.75 }));
}

TEST(vector, equality_of_mixed_types)
{
    zx::mat::vector<int, 3> a{ 1, 2, 3 };
    zx::mat::vector<double, 3> b{ 1.0, 2.0, 3.0 };
    zx::mat::vector<double, 3> c{ 4.0, 5.0, 6.0 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}
