#include <gmock/gmock.h>

#include <type_traits>
#include <zx/md_array.hpp>

TEST(md_array, shape_with_all_static_dims)
{
    using shape_type = zx::mat2::shape_t<zx::mat2::dim_t<3, 8>, zx::mat2::dim_t<4, 2>>;
    shape_type shape;
    EXPECT_THAT(shape.is_static(), true);
    EXPECT_THAT(shape.volume(), 12);
    EXPECT_THAT(shape.flat_offset({ 0, 0 }), zx::mat2::flat_offset_t{ 0 });
    EXPECT_THAT(shape.flat_offset({ 0, 1 }), zx::mat2::flat_offset_t{ 2 });
    EXPECT_THAT(shape.flat_offset({ 0, 2 }), zx::mat2::flat_offset_t{ 4 });
    EXPECT_THAT(shape.flat_offset({ 0, 3 }), zx::mat2::flat_offset_t{ 6 });

    EXPECT_THAT(shape.flat_offset({ 1, 0 }), zx::mat2::flat_offset_t{ 8 });
    EXPECT_THAT(shape.flat_offset({ 1, 1 }), zx::mat2::flat_offset_t{ 10 });
    EXPECT_THAT(shape.flat_offset({ 1, 2 }), zx::mat2::flat_offset_t{ 12 });
    EXPECT_THAT(shape.flat_offset({ 1, 3 }), zx::mat2::flat_offset_t{ 14 });
}

TEST(md_array, shape_from_extent)
{
    EXPECT_THAT(zx::mat2::shape_from_extent(zx::mat2::vec_t{ 3, 4 }, 4), testing::_);
}

TEST(md_array, static_shape_builder)
{
    using built_shape = zx::mat2::shape_from_extent_t<zx::mat2::extent_t<3, 4, 5>, 4>;
    using expected_shape = zx::mat2::shape_t<zx::mat2::dim_t<3, 80>, zx::mat2::dim_t<4, 20>, zx::mat2::dim_t<5, 4>>;
    static_assert(std::is_same_v<built_shape, expected_shape>);
}

TEST(md_array, array_view_of_static_shape_has_static_instance_of_shape)
{
    using shape_type = zx::mat2::shape_t<zx::mat2::dim_t<3, 8>, zx::mat2::dim_t<4, 2>>;
    std::vector<std::int16_t> data(12);
    data[0] = 101;
    data[1] = 102;
    zx::mat2::array_view_t<shape_type, std::int16_t> view{ shape_type{}, data.data() };

    auto copy = view;
    EXPECT_THAT(copy.shape(), testing::Address(&view.shape()));

    EXPECT_THAT((view[{ 0, 0 }]), 101);
    EXPECT_THAT((view[{ 0, 1 }]), 102);
}

TEST(md_array, array_1d)
{
    using shape_type = zx::mat2::shape_t<zx::mat2::dim_t<3, 4>>;
    zx::mat2::array_t<shape_type, std::int32_t> array{};
    array[{ 0 }] = 101;
    EXPECT_THAT((array[{ 0 }]), 101);
    array.view()[{ 1 }] = 202;

    EXPECT_THAT(
        array,
        testing::AllOf(
            testing::Property("volume", &zx::mat2::array_t<shape_type, std::int32_t>::volume, 3),
            testing::Property(
                "bounds", &zx::mat2::array_t<shape_type, std::int32_t>::bounds, zx::mat2::interval_base_t{ 0, 3 })));
}

TEST(md_array, static_array_2d)
{
    zx::mat2::dense_matrix_t<3, 4, int> array{};
    using array_type = decltype(array);
    array[{ 0, 0 }] = 101;
    EXPECT_THAT((array[{ 0, 0 }]), 101);
    array.view()[{ 1, 2 }] = 202;

    EXPECT_THAT(
        array,
        testing::AllOf(
            testing::Property("volume", &array_type::volume, 12),
            testing::Property(
                "bounds",
                &array_type::bounds,
                typename array_type::bounds_type{ zx::mat2::interval_base_t{ 0, 3 }, zx::mat2::interval_base_t{ 0, 4 } })));

    EXPECT_THAT(
        array.view().slice(array_type::slice_type{ zx::mat2::slice_base_t{ 1, 3 }, zx::mat2::slice_base_t{ 2, 4 } }),
        testing::AllOf(
            testing::Property("volume", &array_type::dynamic_view_type::volume, 4),
            testing::Property(
                "bounds",
                &array_type::dynamic_view_type::bounds,
                typename array_type::dynamic_view_type::bounds_type{ zx::mat2::interval_base_t{ 0, 2 },
                                                                     zx::mat2::interval_base_t{ 0, 2 } })));
}

TEST(md_array, dense_vector_times_dense_matrix)
{
    zx::mat2::dense_vector_t<3, int> vector{ 1, 2, 3 };
    zx::mat2::dense_matrix_t<4, 4, int> matrix{};
    matrix[{ 0, 0 }] = 1;
    matrix[{ 1, 1 }] = 1;
    matrix[{ 2, 2 }] = 1;
    matrix[{ 3, 0 }] = 10;
    matrix[{ 3, 1 }] = 20;
    matrix[{ 3, 2 }] = 30;
    matrix[{ 3, 3 }] = 1;

    EXPECT_THAT((vector * matrix), (zx::mat2::dense_vector_t<3, int>{ 11, 22, 33 }));
}

TEST(md_array, dense_matrix_times_dense_matrix)
{
    zx::mat2::dense_matrix_t<2, 3, int> lhs{};
    lhs[{ 0, 0 }] = 1;
    lhs[{ 0, 1 }] = 2;
    lhs[{ 0, 2 }] = 3;
    lhs[{ 1, 0 }] = 4;
    lhs[{ 1, 1 }] = 5;
    lhs[{ 1, 2 }] = 6;

    zx::mat2::dense_matrix_t<3, 2, int> rhs{};
    rhs[{ 0, 0 }] = 7;
    rhs[{ 0, 1 }] = 8;
    rhs[{ 1, 0 }] = 9;
    rhs[{ 1, 1 }] = 10;
    rhs[{ 2, 0 }] = 11;
    rhs[{ 2, 1 }] = 12;

    const auto result = lhs * rhs;
    EXPECT_THAT((result[{ 0, 0 }]), 58);
    EXPECT_THAT((result[{ 0, 1 }]), 64);
    EXPECT_THAT((result[{ 1, 0 }]), 139);
    EXPECT_THAT((result[{ 1, 1 }]), 154);
}

TEST(md_array, dense_matrix_addition)
{
    zx::mat2::dense_matrix_t<2, 2, int> lhs{};
    lhs[{ 0, 0 }] = 1;
    lhs[{ 0, 1 }] = 2;
    lhs[{ 1, 0 }] = 3;
    lhs[{ 1, 1 }] = 4;

    zx::mat2::dense_matrix_t<2, 2, int> rhs{};
    rhs[{ 0, 0 }] = 5;
    rhs[{ 0, 1 }] = 6;
    rhs[{ 1, 0 }] = 7;
    rhs[{ 1, 1 }] = 8;

    const auto result = lhs + rhs;
    EXPECT_THAT((result[{ 0, 0 }]), 6);
    EXPECT_THAT((result[{ 0, 1 }]), 8);
    EXPECT_THAT((result[{ 1, 0 }]), 10);
    EXPECT_THAT((result[{ 1, 1 }]), 12);
}

TEST(md_array, dense_matrix_subtraction)
{
    zx::mat2::dense_matrix_t<2, 2, int> lhs{};
    lhs[{ 0, 0 }] = 9;
    lhs[{ 0, 1 }] = 8;
    lhs[{ 1, 0 }] = 7;
    lhs[{ 1, 1 }] = 6;

    zx::mat2::dense_matrix_t<2, 2, int> rhs{};
    rhs[{ 0, 0 }] = 1;
    rhs[{ 0, 1 }] = 2;
    rhs[{ 1, 0 }] = 3;
    rhs[{ 1, 1 }] = 4;

    const auto result = lhs - rhs;
    EXPECT_THAT((result[{ 0, 0 }]), 8);
    EXPECT_THAT((result[{ 0, 1 }]), 6);
    EXPECT_THAT((result[{ 1, 0 }]), 4);
    EXPECT_THAT((result[{ 1, 1 }]), 2);
}

TEST(md_array, dense_matrix_scalar_multiplication)
{
    zx::mat2::dense_matrix_t<2, 2, int> matrix{};
    matrix[{ 0, 0 }] = 1;
    matrix[{ 0, 1 }] = 2;
    matrix[{ 1, 0 }] = 3;
    matrix[{ 1, 1 }] = 4;

    const auto left = matrix * 3;
    EXPECT_THAT((left[{ 0, 0 }]), 3);
    EXPECT_THAT((left[{ 0, 1 }]), 6);
    EXPECT_THAT((left[{ 1, 0 }]), 9);
    EXPECT_THAT((left[{ 1, 1 }]), 12);

    const auto right = 3 * matrix;
    EXPECT_THAT((right[{ 0, 0 }]), 3);
    EXPECT_THAT((right[{ 0, 1 }]), 6);
    EXPECT_THAT((right[{ 1, 0 }]), 9);
    EXPECT_THAT((right[{ 1, 1 }]), 12);
}

TEST(md_array, dense_matrix_scalar_division)
{
    zx::mat2::dense_matrix_t<2, 2, int> matrix{};
    matrix[{ 0, 0 }] = 4;
    matrix[{ 0, 1 }] = 8;
    matrix[{ 1, 0 }] = 12;
    matrix[{ 1, 1 }] = 16;

    const auto result = matrix / 4;
    EXPECT_THAT((result[{ 0, 0 }]), 1);
    EXPECT_THAT((result[{ 0, 1 }]), 2);
    EXPECT_THAT((result[{ 1, 0 }]), 3);
    EXPECT_THAT((result[{ 1, 1 }]), 4);
}
