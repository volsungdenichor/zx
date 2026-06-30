#include <gmock/gmock.h>

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
