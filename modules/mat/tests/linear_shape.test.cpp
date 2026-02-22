#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(segment, ostream)
{
    zx::mat::segment_t<float, 3> seg{ { 1.0f, 2.0f, 3.0f }, { 4.0f, 5.0f, 6.0f } };

    std::ostringstream os;
    os << seg;

    EXPECT_THAT(os.str(), "(segment [1 2 3] [4 5 6])");
}

TEST(segment, add_vector)
{
    zx::mat::segment_t<float, 2> seg{ { 1.0f, 2.0f }, { 3.0f, 4.0f } };
    zx::mat::vector_t<float, 2> vec{ 10.0f, 20.0f };

    EXPECT_THAT(
        seg + vec,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 11.0f, 22.0f }), (zx::mat::vector_t<float, 2>{ 13.0f, 24.0f })));
}

TEST(segment, subtract_vector)
{
    zx::mat::segment_t<float, 2> seg{ { 10.0f, 20.0f }, { 30.0f, 40.0f } };
    zx::mat::vector_t<float, 2> vec{ 1.0f, 2.0f };

    EXPECT_THAT(
        seg - vec,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 9.0f, 18.0f }), (zx::mat::vector_t<float, 2>{ 29.0f, 38.0f })));
}

TEST(segment, multiply_matrix)
{
    zx::mat::segment_t<int, 2> seg{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<float, 3> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        seg * mat,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 2.0f, 6.0f }), (zx::mat::vector_t<float, 2>{ 6.0f, 12.0f })));

    EXPECT_THAT(
        mat * seg,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 2.0f, 6.0f }), (zx::mat::vector_t<float, 2>{ 6.0f, 12.0f })));
}

TEST(segment, equal)
{
    zx::mat::segment_t<int, 2> seg1{ { 1, 2 }, { 3, 4 } };
    zx::mat::segment_t<int, 2> seg2{ { 1, 2 }, { 3, 4 } };
    zx::mat::segment_t<int, 2> seg3{ { 5, 6 }, { 7, 8 } };

    EXPECT_TRUE(seg1 == seg2);
    EXPECT_FALSE(seg1 != seg2);
    EXPECT_FALSE(seg1 == seg3);
    EXPECT_TRUE(seg1 != seg3);
}

TEST(ray, ostream)
{
    zx::mat::ray_t<double, 2> r{ { 0.0, 0.0 }, { 1.0, 1.0 } };

    std::ostringstream os;
    os << r;

    EXPECT_THAT(os.str(), "(ray [0 0] (dir [1 1]))");
}

TEST(ray, add_vector)
{
    zx::mat::ray_t<double, 2> r{ { 1.0, 2.0 }, { 3.0, 4.0 } };
    zx::mat::vector_t<double, 2> v{ 10.0, 20.0 };

    EXPECT_THAT(
        r + v,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 11.0, 22.0 }), (zx::mat::vector_t<double, 2>{ 13.0, 24.0 })));
}

TEST(ray, subtract_vector)
{
    zx::mat::ray_t<double, 2> r{ { 10.0, 20.0 }, { 30.0, 40.0 } };
    zx::mat::vector_t<double, 2> v{ 1.0, 2.0 };

    EXPECT_THAT(
        r - v,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 9.0, 18.0 }), (zx::mat::vector_t<double, 2>{ 29.0, 38.0 })));
}

TEST(ray, multiply_matrix)
{
    zx::mat::ray_t<int, 2> r{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<double, 3> m{ 2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 1.0 };

    EXPECT_THAT(
        r * m,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 2.0, 6.0 }), (zx::mat::vector_t<double, 2>{ 6.0, 12.0 })));

    EXPECT_THAT(
        m * r,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 2.0, 6.0 }), (zx::mat::vector_t<double, 2>{ 6.0, 12.0 })));
}

TEST(ray, equal)
{
    zx::mat::ray_t<int, 2> r1{ { 1, 2 }, { 3, 4 } };
    zx::mat::ray_t<int, 2> r2{ { 1, 2 }, { 3, 4 } };
    zx::mat::ray_t<int, 2> r3{ { 5, 6 }, { 7, 8 } };

    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
    EXPECT_FALSE(r1 == r3);
    EXPECT_TRUE(r1 != r3);
}

TEST(line, ostream)
{
    zx::mat::line_t<int, 3> line{ { 1, 2, 3 }, { 4, 5, 6 } };

    std::ostringstream os;
    os << line;

    EXPECT_THAT(os.str(), "(line [1 2 3] (dir [3 3 3]))");
}

TEST(line, equal)
{
    zx::mat::line_t<int, 2> line1{ { 1, 2 }, { 3, 4 } };
    zx::mat::line_t<int, 2> line2{ { 1, 2 }, { 3, 4 } };
    zx::mat::line_t<int, 2> line3{ { 5, 6 }, { 7, 8 } };

    EXPECT_TRUE(line1 == line2);
    EXPECT_FALSE(line1 != line2);
    EXPECT_FALSE(line1 == line3);
    EXPECT_TRUE(line1 != line3);
}

TEST(line, add_vector)
{
    zx::mat::line_t<double, 2> line{ { 1.0, 2.0 }, { 3.0, 4.0 } };
    zx::mat::vector_t<double, 2> vec{ 10.0, 20.0 };

    EXPECT_THAT(
        line + vec,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 11.0, 22.0 }), (zx::mat::vector_t<double, 2>{ 13.0, 24.0 })));
}

TEST(line, subtract_vector)
{
    zx::mat::line_t<double, 2> line{ { 10.0, 20.0 }, { 30.0, 40.0 } };
    zx::mat::vector_t<double, 2> vec{ 1.0, 2.0 };

    EXPECT_THAT(
        line - vec,
        testing::ElementsAre((zx::mat::vector_t<double, 2>{ 9.0, 18.0 }), (zx::mat::vector_t<double, 2>{ 29.0, 38.0 })));
}

TEST(line, multiply_matrix)
{
    zx::mat::line_t<int, 2> line{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<float, 3> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        line * mat,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 2.0f, 6.0f }), (zx::mat::vector_t<float, 2>{ 6.0f, 12.0f })));

    EXPECT_THAT(
        mat * line,
        testing::ElementsAre((zx::mat::vector_t<float, 2>{ 2.0f, 6.0f }), (zx::mat::vector_t<float, 2>{ 6.0f, 12.0f })));
}
