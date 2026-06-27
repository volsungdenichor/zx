#include <gmock/gmock.h>

#include <zx/mat.hpp>

#include "matchers.hpp"

TEST(triangle_t, construct_with_arguments)
{
    zx::mat::triangle_t<2, int> t(
        zx::mat::vector_t<2, int>{ 1, 2 }, zx::mat::vector_t<2, int>{ 3, 4 }, zx::mat::vector_t<2, int>{ 5, 6 });

    EXPECT_THAT(
        t,
        testing::ElementsAre(
            (zx::mat::vector_t<2, int>{ 1, 2 }), (zx::mat::vector_t<2, int>{ 3, 4 }), (zx::mat::vector_t<2, int>{ 5, 6 })));
}

TEST(triangle_t, ostream)
{
    zx::mat::triangle_t<3, int> t{ zx::mat::vector_t<3, int>{ 1, 2, 3 },
                                   zx::mat::vector_t<3, int>{ 4, 5, 6 },
                                   zx::mat::vector_t<3, int>{ 7, 8, 9 } };

    std::ostringstream os;
    os << t;

    EXPECT_THAT(os.str(), "([1 2 3] [4 5 6] [7 8 9])");
}

TEST(triangle_t, add_vector)
{
    zx::mat::triangle_t<2, double> t{ zx::mat::vector_t<2, double>{ 1.0, 2.0 },
                                      zx::mat::vector_t<2, double>{ 3.0, 4.0 },
                                      zx::mat::vector_t<2, double>{ 5.0, 6.0 } };
    zx::mat::vector_t<2, double> vec{ 10.0, 20.0 };

    EXPECT_THAT(
        t + vec,
        testing::ElementsAre(
            (zx::mat::vector_t<2, double>{ 11.0, 22.0 }),
            (zx::mat::vector_t<2, double>{ 13.0, 24.0 }),
            (zx::mat::vector_t<2, double>{ 15.0, 26.0 })));
}

TEST(triangle_t, subtract_vector)
{
    zx::mat::triangle_t<2, float> t{ zx::mat::vector_t<2, float>{ 10.0f, 20.0f },
                                     zx::mat::vector_t<2, float>{ 30.0f, 40.0f },
                                     zx::mat::vector_t<2, float>{ 50.0f, 60.0f } };
    zx::mat::vector_t<2, float> vec{ 1.0f, 2.0f };

    EXPECT_THAT(
        t - vec,
        testing::ElementsAre(
            (zx::mat::vector_t<2, float>{ 9.0f, 18.0f }),
            (zx::mat::vector_t<2, float>{ 29.0f, 38.0f }),
            (zx::mat::vector_t<2, float>{ 49.0f, 58.0f })));
}

TEST(triangle_t, equal)
{
    zx::mat::triangle_t<2, int> tri1{ zx::mat::vector_t<2, int>{ 1, 2 },
                                      zx::mat::vector_t<2, int>{ 3, 4 },
                                      zx::mat::vector_t<2, int>{ 5, 6 } };
    zx::mat::triangle_t<2, int> tri2{ zx::mat::vector_t<2, int>{ 1, 2 },
                                      zx::mat::vector_t<2, int>{ 3, 4 },
                                      zx::mat::vector_t<2, int>{ 5, 6 } };
    zx::mat::triangle_t<2, int> tri3{ zx::mat::vector_t<2, int>{ 7, 8 },
                                      zx::mat::vector_t<2, int>{ 9, 10 },
                                      zx::mat::vector_t<2, int>{ 11, 12 } };

    EXPECT_THAT(tri1 == tri2, testing::IsTrue());
    EXPECT_THAT(tri1 != tri2, testing::IsFalse());
    EXPECT_THAT(tri1 == tri3, testing::IsFalse());
    EXPECT_THAT(tri1 != tri3, testing::IsTrue());
}

TEST(triangle_t, multiply_matrix)
{
    zx::mat::triangle_t<2, int> t{ zx::mat::vector_t<2, int>{ 1, 2 },
                                   zx::mat::vector_t<2, int>{ 3, 4 },
                                   zx::mat::vector_t<2, int>{ 5, 6 } };
    zx::mat::matrix_t<3, 3, float> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        t * mat,
        testing::ElementsAre(
            (zx::mat::vector_t<2, float>{ 2.0f, 6.0f }),
            (zx::mat::vector_t<2, float>{ 6.0f, 12.0f }),
            (zx::mat::vector_t<2, float>{ 10.0f, 18.0f })));

    EXPECT_THAT(
        mat * t,
        testing::ElementsAre(
            (zx::mat::vector_t<2, float>{ 2.0f, 6.0f }),
            (zx::mat::vector_t<2, float>{ 6.0f, 12.0f }),
            (zx::mat::vector_t<2, float>{ 10.0f, 18.0f })));
}

TEST(quad_t, ostream)
{
    zx::mat::quad_t<2, int> q{ zx::mat::vector_t<2, int>{ 1, 2 },
                               zx::mat::vector_t<2, int>{ 3, 4 },
                               zx::mat::vector_t<2, int>{ 5, 6 },
                               zx::mat::vector_t<2, int>{ 7, 8 } };

    std::ostringstream os;
    os << q;

    EXPECT_THAT(os.str(), "([1 2] [3 4] [5 6] [7 8])");
}

TEST(quad_t, add_vector)
{
    zx::mat::quad_t<3, float> q{ zx::mat::vector_t<3, float>{ 1.0f, 2.0f, 3.0f },
                                 zx::mat::vector_t<3, float>{ 4.0f, 5.0f, 6.0f },
                                 zx::mat::vector_t<3, float>{ 7.0f, 8.0f, 9.0f },
                                 zx::mat::vector_t<3, float>{ 10.0f, 11.0f, 12.0f } };
    zx::mat::vector_t<3, float> vec{ 10.0f, 20.0f, 30.0f };

    EXPECT_THAT(
        q + vec,
        testing::ElementsAre(
            (zx::mat::vector_t<3, float>{ 11.0f, 22.0f, 33.0f }),
            (zx::mat::vector_t<3, float>{ 14.0f, 25.0f, 36.0f }),
            (zx::mat::vector_t<3, float>{ 17.0f, 28.0f, 39.0f }),
            (zx::mat::vector_t<3, float>{ 20.0f, 31.0f, 42.0f })));
}

TEST(quad_t, subtract_vector)
{
    zx::mat::quad_t<2, int> q{ zx::mat::vector_t<2, int>{ 10, 20 },
                               zx::mat::vector_t<2, int>{ 30, 40 },
                               zx::mat::vector_t<2, int>{ 50, 60 },
                               zx::mat::vector_t<2, int>{ 70, 80 } };
    zx::mat::vector_t<2, int> vec{ 1, 2 };

    EXPECT_THAT(
        q - vec,
        testing::ElementsAre(
            (zx::mat::vector_t<2, int>{ 9, 18 }),
            (zx::mat::vector_t<2, int>{ 29, 38 }),
            (zx::mat::vector_t<2, int>{ 49, 58 }),
            (zx::mat::vector_t<2, int>{ 69, 78 })));
}

TEST(quad_t, equal)
{
    zx::mat::quad_t<2, double> q1{ zx::mat::vector_t<2, double>{ 1.0, 2.0 },
                                   zx::mat::vector_t<2, double>{ 3.0, 4.0 },
                                   zx::mat::vector_t<2, double>{ 5.0, 6.0 },
                                   zx::mat::vector_t<2, double>{ 7.0, 8.0 } };
    zx::mat::quad_t<2, double> q2{ zx::mat::vector_t<2, double>{ 1.0, 2.0 },
                                   zx::mat::vector_t<2, double>{ 3.0, 4.0 },
                                   zx::mat::vector_t<2, double>{ 5.0, 6.0 },
                                   zx::mat::vector_t<2, double>{ 7.0, 8.0 } };
    zx::mat::quad_t<2, double> q3{ zx::mat::vector_t<2, double>{ 9.0, 10.0 },
                                   zx::mat::vector_t<2, double>{ 11.0, 12.0 },
                                   zx::mat::vector_t<2, double>{ 13.0, 14.0 },
                                   zx::mat::vector_t<2, double>{ 15.0, 16.0 } };

    EXPECT_THAT(q1 == q2, testing::IsTrue());
    EXPECT_THAT(q1 != q2, testing::IsFalse());
    EXPECT_THAT(q1 == q3, testing::IsFalse());
    EXPECT_THAT(q1 != q3, testing::IsTrue());
}

TEST(quad_t, multiply_matrix)
{
    zx::mat::quad_t<2, int> q{ zx::mat::vector_t<2, int>{ 1, 2 },
                               zx::mat::vector_t<2, int>{ 3, 4 },
                               zx::mat::vector_t<2, int>{ 5, 6 },
                               zx::mat::vector_t<2, int>{ 7, 8 } };
    zx::mat::matrix_t<3, 3, float> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        q * mat,
        testing::ElementsAre(
            (zx::mat::vector_t<2, float>{ 2.0f, 6.0f }),
            (zx::mat::vector_t<2, float>{ 6.0f, 12.0f }),
            (zx::mat::vector_t<2, float>{ 10.0f, 18.0f }),
            (zx::mat::vector_t<2, float>{ 14.0f, 24.0f })));

    EXPECT_THAT(
        mat * q,
        testing::ElementsAre(
            (zx::mat::vector_t<2, float>{ 2.0f, 6.0f }),
            (zx::mat::vector_t<2, float>{ 6.0f, 12.0f }),
            (zx::mat::vector_t<2, float>{ 10.0f, 18.0f }),
            (zx::mat::vector_t<2, float>{ 14.0f, 24.0f })));
}
