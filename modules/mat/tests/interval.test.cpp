#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(interval, default_construction)
{
    zx::mat::interval<int> iv;

    EXPECT_THAT(iv, (zx::mat::interval<int>{ 0, 0 }));
}

TEST(interval, parameterized_construction)
{
    zx::mat::interval<int> iv{ 1, 5 };

    EXPECT_THAT(iv, (zx::mat::interval<int>{ 1, 5 }));
}

TEST(interval, addition)
{
    zx::mat::interval<int> iv{ 1, 5 };

    EXPECT_THAT(iv + 3, (zx::mat::interval<int>{ 4, 8 }));

    iv += 2;
    EXPECT_THAT(iv, (zx::mat::interval<int>{ 3, 7 }));
}

TEST(interval, subtraction)
{
    zx::mat::interval<int> iv{ 5, 10 };

    EXPECT_THAT(iv - 2, (zx::mat::interval<int>{ 3, 8 }));

    iv -= 3;
    EXPECT_THAT(iv, (zx::mat::interval<int>{ 2, 7 }));
}

TEST(interval, equality)
{
    zx::mat::interval<int> a{ 1, 5 };
    zx::mat::interval<int> b{ 1, 5 };
    zx::mat::interval<int> c{ 2, 6 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(interval, output_stream)
{
    zx::mat::interval<int> iv{ 1, 5 };
    std::ostringstream os;
    os << iv;
    EXPECT_THAT(os.str(), "[1 5)");
}
