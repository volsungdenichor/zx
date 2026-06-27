#include <gmock/gmock.h>

#include <zx/mat.hpp>

TEST(segment, ostream)
{
    zx::mat::segment_t<3, float> seg{ { 1.0f, 2.0f, 3.0f }, { 4.0f, 5.0f, 6.0f } };

    std::ostringstream os;
    os << seg;

    EXPECT_THAT(os.str(), "(segment [1 2 3] [4 5 6])");
}

TEST(segment, add_vector)
{
    zx::mat::segment_t<2, float> seg{ { 1.0f, 2.0f }, { 3.0f, 4.0f } };
    zx::mat::vector_t<2, float> vec{ 10.0f, 20.0f };

    EXPECT_THAT(
        seg + vec,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 11.0f, 22.0f }), (zx::mat::vector_t<2, float>{ 13.0f, 24.0f })));
}

TEST(segment, subtract_vector)
{
    zx::mat::segment_t<2, float> seg{ { 10.0f, 20.0f }, { 30.0f, 40.0f } };
    zx::mat::vector_t<2, float> vec{ 1.0f, 2.0f };

    EXPECT_THAT(
        seg - vec,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 9.0f, 18.0f }), (zx::mat::vector_t<2, float>{ 29.0f, 38.0f })));
}

TEST(segment, multiply_matrix)
{
    zx::mat::segment_t<2, int> seg{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<3, 3, float> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        seg * mat,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 2.0f, 6.0f }), (zx::mat::vector_t<2, float>{ 6.0f, 12.0f })));

    EXPECT_THAT(
        mat * seg,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 2.0f, 6.0f }), (zx::mat::vector_t<2, float>{ 6.0f, 12.0f })));
}

TEST(segment, equal)
{
    zx::mat::segment_t<2, int> seg1{ { 1, 2 }, { 3, 4 } };
    zx::mat::segment_t<2, int> seg2{ { 1, 2 }, { 3, 4 } };
    zx::mat::segment_t<2, int> seg3{ { 5, 6 }, { 7, 8 } };

    EXPECT_THAT(seg1 == seg2, testing::IsTrue());
    EXPECT_THAT(seg1 != seg2, testing::IsFalse());
    EXPECT_THAT(seg1 == seg3, testing::IsFalse());
    EXPECT_THAT(seg1 != seg3, testing::IsTrue());
}

TEST(ray, ostream)
{
    zx::mat::ray_t<2, double> r{ { 0.0, 0.0 }, { 1.0, 1.0 } };

    std::ostringstream os;
    os << r;

    EXPECT_THAT(os.str(), "(ray [0 0] (dir [1 1]))");
}

TEST(ray, add_vector)
{
    zx::mat::ray_t<2, double> r{ { 1.0, 2.0 }, { 3.0, 4.0 } };
    zx::mat::vector_t<2, double> v{ 10.0, 20.0 };

    EXPECT_THAT(
        r + v,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 11.0, 22.0 }), (zx::mat::vector_t<2, double>{ 13.0, 24.0 })));
}

TEST(ray, subtract_vector)
{
    zx::mat::ray_t<2, double> r{ { 10.0, 20.0 }, { 30.0, 40.0 } };
    zx::mat::vector_t<2, double> v{ 1.0, 2.0 };

    EXPECT_THAT(
        r - v,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 9.0, 18.0 }), (zx::mat::vector_t<2, double>{ 29.0, 38.0 })));
}

TEST(ray, multiply_matrix)
{
    zx::mat::ray_t<2, int> r{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<3, 3, double> m{ 2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 1.0 };

    EXPECT_THAT(
        r * m,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 2.0, 6.0 }), (zx::mat::vector_t<2, double>{ 6.0, 12.0 })));

    EXPECT_THAT(
        m * r,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 2.0, 6.0 }), (zx::mat::vector_t<2, double>{ 6.0, 12.0 })));
}

TEST(ray, equal)
{
    zx::mat::ray_t<2, int> r1{ { 1, 2 }, { 3, 4 } };
    zx::mat::ray_t<2, int> r2{ { 1, 2 }, { 3, 4 } };
    zx::mat::ray_t<2, int> r3{ { 5, 6 }, { 7, 8 } };

    EXPECT_THAT(r1 == r2, testing::IsTrue());
    EXPECT_THAT(r1 != r2, testing::IsFalse());
    EXPECT_THAT(r1 == r3, testing::IsFalse());
    EXPECT_THAT(r1 != r3, testing::IsTrue());
}

TEST(line, ostream)
{
    zx::mat::line_t<3, int> line{ { 1, 2, 3 }, { 4, 5, 6 } };

    std::ostringstream os;
    os << line;

    EXPECT_THAT(os.str(), "(line [1 2 3] (dir [3 3 3]))");
}

TEST(line, equal)
{
    zx::mat::line_t<2, int> line1{ { 1, 2 }, { 3, 4 } };
    zx::mat::line_t<2, int> line2{ { 1, 2 }, { 3, 4 } };
    zx::mat::line_t<2, int> line3{ { 5, 6 }, { 7, 8 } };

    EXPECT_THAT(line1 == line2, testing::IsTrue());
    EXPECT_THAT(line1 != line2, testing::IsFalse());
    EXPECT_THAT(line1 == line3, testing::IsFalse());
    EXPECT_THAT(line1 != line3, testing::IsTrue());
}

TEST(line, add_vector)
{
    zx::mat::line_t<2, double> line{ { 1.0, 2.0 }, { 3.0, 4.0 } };
    zx::mat::vector_t<2, double> vec{ 10.0, 20.0 };

    EXPECT_THAT(
        line + vec,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 11.0, 22.0 }), (zx::mat::vector_t<2, double>{ 13.0, 24.0 })));
}

TEST(line, subtract_vector)
{
    zx::mat::line_t<2, double> line{ { 10.0, 20.0 }, { 30.0, 40.0 } };
    zx::mat::vector_t<2, double> vec{ 1.0, 2.0 };

    EXPECT_THAT(
        line - vec,
        testing::ElementsAre((zx::mat::vector_t<2, double>{ 9.0, 18.0 }), (zx::mat::vector_t<2, double>{ 29.0, 38.0 })));
}

TEST(line, multiply_matrix)
{
    zx::mat::line_t<2, int> line{ { 1, 2 }, { 3, 4 } };
    zx::mat::matrix_t<3, 3, float> mat{ 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    EXPECT_THAT(
        line * mat,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 2.0f, 6.0f }), (zx::mat::vector_t<2, float>{ 6.0f, 12.0f })));

    EXPECT_THAT(
        mat * line,
        testing::ElementsAre((zx::mat::vector_t<2, float>{ 2.0f, 6.0f }), (zx::mat::vector_t<2, float>{ 6.0f, 12.0f })));
}
