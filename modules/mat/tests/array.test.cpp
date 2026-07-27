#include <gmock/gmock.h>

#include <zx/array.hpp>

template <class T>
zx::mat::stride_base_t stride_of(zx::mat::location_base_t n)
{
    return static_cast<zx::mat::stride_base_t>(n * static_cast<zx::mat::stride_base_t>(sizeof(T)));
}

TEST(array, slicing)
{
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ {}, {}, {} })), testing::FieldsAre(zx::mat::dim_t{ 10, 1 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 3, {}, {} })), testing::FieldsAre(zx::mat::dim_t{ 7, 1 }, 3));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ {}, 7, {} })), testing::FieldsAre(zx::mat::dim_t{ 7, 1 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 2, 8, {} })), testing::FieldsAre(zx::mat::dim_t{ 6, 1 }, 2));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 0, 10, 3 })), testing::FieldsAre(zx::mat::dim_t{ 4, 3 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 2, 8, 2 })), testing::FieldsAre(zx::mat::dim_t{ 3, 2 }, 2));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ -8, -2, {} })), testing::FieldsAre(zx::mat::dim_t{ 6, 1 }, 2));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ -5, {}, {} })), testing::FieldsAre(zx::mat::dim_t{ 5, 1 }, 5));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ {}, -3, {} })), testing::FieldsAre(zx::mat::dim_t{ 7, 1 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ {}, {}, -1 })), testing::FieldsAre(zx::mat::dim_t{ 10, -1 }, 9));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 8, 2, -2 })), testing::FieldsAre(zx::mat::dim_t{ 3, -2 }, 8));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 9, {}, -1 })), testing::FieldsAre(zx::mat::dim_t{ 10, -1 }, 9));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 0, 100, {} })), testing::FieldsAre(zx::mat::dim_t{ 10, 1 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ -100, 5, {} })), testing::FieldsAre(zx::mat::dim_t{ 5, 1 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 5, 5, {} })), testing::FieldsAre(zx::mat::dim_t{ 0, 1 }, 5));
    EXPECT_THAT((zx::mat::dim_t{ 10, 2 }.slice({ 1, 5, {} })), testing::FieldsAre(zx::mat::dim_t{ 4, 2 }, 2));
    EXPECT_THAT((zx::mat::dim_t{ 10, 2 }.slice({ 0, 10, 2 })), testing::FieldsAre(zx::mat::dim_t{ 5, 4 }, 0));
    EXPECT_THAT((zx::mat::dim_t{ 10, 1 }.slice({ 2, 8, {} })), testing::FieldsAre(zx::mat::dim_t{ 6, 1 }, 2));
}

TEST(array, empty_array_1d_slice)
{
    zx::mat::array_t<int, 1> a{ 10 };
    auto view = a.view().slice({ 5, 5 }).slice({ {}, {}, -1 });
    EXPECT_THAT(view.extent(), 0);
    EXPECT_THAT(view.stride(), -stride_of<int>(1));
    EXPECT_THAT(view.volume(), 0);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 0 }));
}

TEST(array, array_1d)
{
    zx::mat::array_t<int, 1> a{ 10 };
    a[1] = 42;
    EXPECT_THAT(
        a.shape(), (zx::mat::shape_t<1>{ { zx::mat::dim_t{ 10, static_cast<zx::mat::stride_base_t>(sizeof(int)) } } }));
    EXPECT_THAT(a.extent(), 10);
    EXPECT_THAT(a.stride(), stride_of<int>(1));
    EXPECT_THAT(a.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 42);

    auto view = a.view();
    EXPECT_THAT(view.extent(), 10);
    EXPECT_THAT(view.stride(), stride_of<int>(1));
    EXPECT_THAT(view.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(view[0], 0);
    EXPECT_THAT(view[1], 42);

    EXPECT_THAT(view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));

    auto mut_view = a.mut_view();
    EXPECT_THAT(mut_view.extent(), 10);
    EXPECT_THAT(mut_view.stride(), stride_of<int>(1));
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(mut_view.volume(), 10);
    EXPECT_THAT(mut_view[0], 0);
    EXPECT_THAT(mut_view[1], 42);

    EXPECT_THAT(mut_view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));
}

TEST(array, array_1d_access)
{
    zx::mat::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 1);
    EXPECT_THAT(a[9], 9);

    EXPECT_THAT(a[-1], 9);
    EXPECT_THAT(a[-2], 8);

    EXPECT_THAT(
        [&] { a[19]; }, testing::ThrowsMessage<std::out_of_range>(testing::HasSubstr("Location 19 is out of bounds (10)")));
}

TEST(array, array_1d_slice)
{
    zx::mat::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 2, 8 });
    EXPECT_THAT(view.extent(), 6);
    EXPECT_THAT(view.stride(), stride_of<int>(1));
    EXPECT_THAT(view.volume(), 6);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 6 }));
    EXPECT_THAT(view[0], 2);
    EXPECT_THAT(view[1], 3);
    EXPECT_THAT(view[2], 4);
    EXPECT_THAT(view[3], 5);
    EXPECT_THAT(view[4], 6);
    EXPECT_THAT(view[5], 7);
    EXPECT_THAT(view, testing::ElementsAreArray({ 2, 3, 4, 5, 6, 7 }));
}

TEST(array, array_1d_slice_step)
{
    zx::mat::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 2, 8, 2 });
    EXPECT_THAT(view.extent(), 3);
    EXPECT_THAT(view.stride(), stride_of<int>(2));
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 3 }));
    EXPECT_THAT(view[0], 2);
    EXPECT_THAT(view[1], 4);
    EXPECT_THAT(view[2], 6);
    EXPECT_THAT(view, testing::ElementsAreArray({ 2, 4, 6 }));
}

TEST(array, array_1d_slice_negative)
{
    zx::mat::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ -8, -2 });
    EXPECT_THAT(view.extent(), 6);
    EXPECT_THAT(view.stride(), stride_of<int>(1));
    EXPECT_THAT(view.volume(), 6);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 6 }));
    EXPECT_THAT(view[0], 2);
    EXPECT_THAT(view[1], 3);
    EXPECT_THAT(view[2], 4);
    EXPECT_THAT(view[3], 5);
    EXPECT_THAT(view[4], 6);
    EXPECT_THAT(view[5], 7);
    EXPECT_THAT(view, testing::ElementsAreArray({ 2, 3, 4, 5, 6, 7 }));
}

TEST(array, array_1d_slice_negative_step)
{
    zx::mat::array_t<int, 1> a{ 10 };
    EXPECT_THAT(a.view().slice({ {}, {}, -1 }).slice({ {}, {}, -1 }).shape(), testing::Eq(a.shape()));
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 8, 2, -2 });
    EXPECT_THAT(view.extent(), 3);
    EXPECT_THAT(view.stride(), -stride_of<int>(2));
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 3 }));
    EXPECT_THAT(view[0], 8);
    EXPECT_THAT(view[1], 6);
    EXPECT_THAT(view[2], 4);
    EXPECT_THAT(view, testing::ElementsAreArray({ 8, 6, 4 }));
}

TEST(array, array_2d_indexing)
{
    zx::mat::array_t<int, 2> a{ { 3, 4 } };
    a[0][0] = 1;
    a[0][1] = 42;
    a[1][0] = 53;
    a[2].fill(-1);
    EXPECT_THAT(a.volume(), 12);
    EXPECT_THAT(a[0][0], 1);
    EXPECT_THAT(a[0][1], 42);
    EXPECT_THAT(a[1][0], 53);
    EXPECT_THAT(a.m_data, testing::ElementsAreArray({ 1, 42, 0, 0, 53, 0, 0, 0, -1, -1, -1, -1 }));
}

TEST(array, array_2d_copy)
{
    zx::mat::array_t<int, 2> a{ { 3, 4 } };
    for (std::size_t i = 0; i < 12; ++i)
    {
        a.m_data[i] = static_cast<int>(i);
    }
    zx::mat::array_t<int, 2> b{ { 3, 4 } };
    zx::mat::copy(b.mut_view(), a.view());
    EXPECT_THAT(b.m_data, testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }));
}

TEST(array, array_3d_assign_from_range_is_flat)
{
    zx::mat::array_t<int, 3> rgb{ { 2, 2, 3 } };
    rgb.mut_view().assign(std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 });

    EXPECT_THAT(rgb.m_data, testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }));
}

TEST(array, array_2d_iteration_is_flat_row_major)
{
    zx::mat::array_t<int, 2> a{ { 2, 3 } };
    a.m_data = { 0, 1, 2, 3, 4, 5 };

    EXPECT_THAT(a, testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5 }));
    EXPECT_THAT(a.view(), testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5 }));
}

TEST(array, array_3d_mutable_iteration_writes_flat_row_major)
{
    zx::mat::array_t<int, 3> a{ { 2, 2, 3 } };

    int v = 0;
    for (auto& item : a.mut_view())
    {
        item = v++;
    }

    EXPECT_THAT(a.m_data, testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }));
}

TEST(array, array_assign_from_long_range_is_clipped)
{
    zx::mat::array_t<int, 1> a{ 5 };
    a.mut_view().assign(std::vector<int>{ 0, 1, 2, 3, 4, 5, 6 });

    EXPECT_THAT(a.m_data, testing::ElementsAreArray({ 0, 1, 2, 3, 4 }));
}

TEST(array, array_assign_from_short_range_overwrites_prefix_only)
{
    zx::mat::array_t<int, 1> a{ 5 };
    a.mut_view().fill(-1);
    a.mut_view().assign(std::vector<int>{ 9, 8, 7 });

    EXPECT_THAT(a.m_data, testing::ElementsAreArray({ 9, 8, 7, -1, -1 }));
}

TEST(array, array_construct_from_view_same_type)
{
    zx::mat::array_t<int, 2> src{ { 3, 4 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i);
    }

    auto src_view = src.view().slice({ { 0, 3, 1 }, { 0, 4, 2 } });
    zx::mat::array_t<int, 2> dst{ src_view };

    EXPECT_THAT(dst.extent(), testing::Eq(src_view.extent()));
    EXPECT_THAT(dst.m_data, testing::ElementsAreArray({ 0, 2, 4, 6, 8, 10 }));
}

TEST(array, array_construct_from_view_convertible_type)
{
    zx::mat::array_t<int, 2> src{ { 2, 3 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i + 1);
    }

    zx::mat::array_t<double, 2> dst{ src.view() };

    EXPECT_THAT(dst.extent(), testing::Eq(src.extent()));
    EXPECT_THAT(dst.m_data, testing::ElementsAreArray({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 }));
}

TEST(array, array_2d_copy_with_positive_location)
{
    zx::mat::array_t<int, 2> src{ { 3, 4 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i);
    }

    zx::mat::array_t<int, 2> dst{ { 4, 5 } };
    dst.mut_view().fill(-1);

    zx::mat::copy(dst.mut_view(), src.view(), { 1, 2 });

    EXPECT_THAT(
        dst.m_data,
        testing::ElementsAreArray({
            -1, -1, -1, -1, -1,  // row 0
            -1, -1, 0,  1,  2,   // row 1
            -1, -1, 4,  5,  6,   // row 2
            -1, -1, 8,  9,  10   // row 3
        }));
}

TEST(array, array_2d_copy_with_negative_location)
{
    zx::mat::array_t<int, 2> src{ { 3, 4 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i);
    }

    zx::mat::array_t<int, 2> dst{ { 4, 5 } };
    dst.mut_view().fill(-1);

    zx::mat::copy(dst.mut_view(), src.view(), { -1, -2 });

    EXPECT_THAT(
        dst.m_data,
        testing::ElementsAreArray({
            6,  7,  -1, -1, -1,  // row 0
            10, 11, -1, -1, -1,  // row 1
            -1, -1, -1, -1, -1,  // row 2
            -1, -1, -1, -1, -1   // row 3
        }));
}

TEST(array, array_2d_copy_bounds_adjustment)
{
    zx::mat::array_t<int, 2> src{ { 3, 4 } };
    zx::mat::array_t<int, 2> dst{ { 4, 5 } };

    const auto [src_bounds, dst_bounds] = zx::mat::adjust_bounds(dst.bounds(), src.bounds(), { -1, 2 });

    EXPECT_THAT(src_bounds[0], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 1, 3 })));
    EXPECT_THAT(src_bounds[1], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 3 })));

    EXPECT_THAT(dst_bounds[0], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 2 })));
    EXPECT_THAT(dst_bounds[1], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 2, 5 })));
}

TEST(array, array_2d_adjust_copy_bounds)
{
    zx::mat::array_t<int, 2> src{ { 3, 4 } };
    zx::mat::array_t<int, 2> dst{ { 4, 5 } };

    const zx::mat::bounds_t<2> src_box{ {
        zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 3 },
        zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 4 },
    } };

    const zx::mat::bounds_t<2> dst_box{ {
        zx::mat::interval_t<zx::mat::extent_base_t>{ -1, 2 },
        zx::mat::interval_t<zx::mat::extent_base_t>{ 2, 6 },
    } };

    const auto [src_bounds, dst_bounds]
        = zx::mat::adjust_copy_bounds(std::pair{ dst.bounds(), dst_box }, std::pair{ src.bounds(), src_box });

    EXPECT_THAT(src_bounds[0], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 1, 3 })));
    EXPECT_THAT(src_bounds[1], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 3 })));

    EXPECT_THAT(dst_bounds[0], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 0, 2 })));
    EXPECT_THAT(dst_bounds[1], testing::Eq((zx::mat::interval_t<zx::mat::extent_base_t>{ 2, 5 })));
}
