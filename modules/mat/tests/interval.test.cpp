#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(interval_t, default_construction)
{
    zx::mat::interval_t<int> iv;

    EXPECT_THAT(iv, (zx::mat::interval_t<int>{ 0, 0 }));
}

TEST(interval_t, parameterized_construction)
{
    zx::mat::interval_t<int> iv{ 1, 5 };

    EXPECT_THAT(iv, (zx::mat::interval_t<int>{ 1, 5 }));
}

TEST(interval_t, addition)
{
    zx::mat::interval_t<int> iv{ 1, 5 };

    EXPECT_THAT(iv + 3, (zx::mat::interval_t<int>{ 4, 8 }));

    iv += 2;
    EXPECT_THAT(iv, (zx::mat::interval_t<int>{ 3, 7 }));
}

TEST(interval_t, subtraction)
{
    zx::mat::interval_t<int> iv{ 5, 10 };

    EXPECT_THAT(iv - 2, (zx::mat::interval_t<int>{ 3, 8 }));

    iv -= 3;
    EXPECT_THAT(iv, (zx::mat::interval_t<int>{ 2, 7 }));
}

TEST(interval_t, equality)
{
    zx::mat::interval_t<int> a{ 1, 5 };
    zx::mat::interval_t<int> b{ 1, 5 };
    zx::mat::interval_t<int> c{ 2, 6 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(interval_t, output_stream)
{
    zx::mat::interval_t<int> iv{ 1, 5 };
    std::ostringstream os;
    os << iv;
    EXPECT_THAT(os.str(), "[1 5)");
}
