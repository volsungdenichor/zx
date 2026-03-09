#include <gmock/gmock.h>

#include <zx/array.hpp>

TEST(array, slicing)
{
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ {}, {}, {} })), testing::FieldsAre(zx::arrays::dim_t{ 10, 1 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 3, {}, {} })), testing::FieldsAre(zx::arrays::dim_t{ 7, 1 }, 3));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ {}, 7, {} })), testing::FieldsAre(zx::arrays::dim_t{ 7, 1 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 2, 8, {} })), testing::FieldsAre(zx::arrays::dim_t{ 6, 1 }, 2));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 0, 10, 3 })), testing::FieldsAre(zx::arrays::dim_t{ 4, 3 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 2, 8, 2 })), testing::FieldsAre(zx::arrays::dim_t{ 3, 2 }, 2));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ -8, -2, {} })), testing::FieldsAre(zx::arrays::dim_t{ 6, 1 }, 2));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ -5, {}, {} })), testing::FieldsAre(zx::arrays::dim_t{ 5, 1 }, 5));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ {}, -3, {} })), testing::FieldsAre(zx::arrays::dim_t{ 7, 1 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ {}, {}, -1 })), testing::FieldsAre(zx::arrays::dim_t{ 10, -1 }, 9));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 8, 2, -2 })), testing::FieldsAre(zx::arrays::dim_t{ 3, -2 }, 8));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 9, {}, -1 })), testing::FieldsAre(zx::arrays::dim_t{ 10, -1 }, 9));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 0, 100, {} })), testing::FieldsAre(zx::arrays::dim_t{ 10, 1 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ -100, 5, {} })), testing::FieldsAre(zx::arrays::dim_t{ 5, 1 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 5, 5, {} })), testing::FieldsAre(zx::arrays::dim_t{ 0, 1 }, 5));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 2 }.slice({ 1, 5, {} })), testing::FieldsAre(zx::arrays::dim_t{ 4, 2 }, 2));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 2 }.slice({ 0, 10, 2 })), testing::FieldsAre(zx::arrays::dim_t{ 5, 4 }, 0));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1 }.slice({ 2, 8, {} })), testing::FieldsAre(zx::arrays::dim_t{ 6, 1 }, 2));
}

TEST(array, empty_array_1d_slice)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    auto view = a.view().slice({ 5, 5 }).slice({ {}, {}, -1 });
    EXPECT_THAT(view.size(), 0);
    EXPECT_THAT(view.stride(), -1);
    EXPECT_THAT(view.volume(), 0);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 0 }));
}

TEST(array, array_1d)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    a[1] = 42;
    EXPECT_THAT(a.shape(), (zx::arrays::shape_t<1>{ { zx::arrays::dim_t{ 10, 1 } } }));
    EXPECT_THAT(a.size(), 10);
    EXPECT_THAT(a.stride(), 1);
    EXPECT_THAT(a.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 42);

    auto view = a.view();
    EXPECT_THAT(view.size(), 10);
    EXPECT_THAT(view.stride(), 1);
    EXPECT_THAT(view.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(view[0], 0);
    EXPECT_THAT(view[1], 42);

    EXPECT_THAT(view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));

    auto mut_view = a.mut_view();
    EXPECT_THAT(mut_view.size(), 10);
    EXPECT_THAT(mut_view.stride(), 1);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(mut_view.volume(), 10);
    EXPECT_THAT(mut_view[0], 0);
    EXPECT_THAT(mut_view[1], 42);

    EXPECT_THAT(mut_view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));
}

TEST(array, array_1d_access)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 1);
    EXPECT_THAT(a[9], 9);

    EXPECT_THAT(a[-1], 9);
    EXPECT_THAT(a[-2], 8);

    EXPECT_THROW(a[19], std::out_of_range);
}

TEST(array, array_1d_slice)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 2, 8 });
    EXPECT_THAT(view.size(), 6);
    EXPECT_THAT(view.stride(), 1);
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
    zx::arrays::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 2, 8, 2 });
    EXPECT_THAT(view.size(), 3);
    EXPECT_THAT(view.stride(), 2);
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 3 }));
    EXPECT_THAT(view[0], 2);
    EXPECT_THAT(view[1], 4);
    EXPECT_THAT(view[2], 6);
    EXPECT_THAT(view, testing::ElementsAreArray({ 2, 4, 6 }));
}

TEST(array, array_1d_slice_negative)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ -8, -2 });
    EXPECT_THAT(view.size(), 6);
    EXPECT_THAT(view.stride(), 1);
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
    zx::arrays::array_t<int, 1> a{ 10 };
    EXPECT_THAT(a.view().slice({ {}, {}, -1 }).slice({ {}, {}, -1 }).shape(), testing::Eq(a.shape()));
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ 8, 2, -2 });
    EXPECT_THAT(view.size(), 3);
    EXPECT_THAT(view.stride(), -2);
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::interval_t<int>{ 0, 3 }));
    EXPECT_THAT(view[0], 8);
    EXPECT_THAT(view[1], 6);
    EXPECT_THAT(view[2], 4);
    EXPECT_THAT(view, testing::ElementsAreArray({ 8, 6, 4 }));
}

TEST(array, array_2d_indexing)
{
    zx::arrays::array_t<int, 2> a{ { 3, 4 } };
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
    zx::arrays::array_t<int, 2> a{ { 3, 4 } };
    for (std::size_t i = 0; i < 12; ++i)
    {
        a.m_data[i] = static_cast<int>(i);
    }
    zx::arrays::array_t<int, 2> b{ { 3, 4 } };
    zx::arrays::copy(b.mut_view(), a.view());
    EXPECT_THAT(b.m_data, testing::ElementsAreArray({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }));
}

TEST(array, array_2d_copy_with_positive_location)
{
    zx::arrays::array_t<int, 2> src{ { 3, 4 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i);
    }

    zx::arrays::array_t<int, 2> dst{ { 4, 5 } };
    dst.mut_view().fill(-1);

    zx::arrays::copy(dst.mut_view(), src.view(), { 1, 2 });

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
    zx::arrays::array_t<int, 2> src{ { 3, 4 } };
    for (std::size_t i = 0; i < src.m_data.size(); ++i)
    {
        src.m_data[i] = static_cast<int>(i);
    }

    zx::arrays::array_t<int, 2> dst{ { 4, 5 } };
    dst.mut_view().fill(-1);

    zx::arrays::copy(dst.mut_view(), src.view(), { -1, -2 });

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
    zx::arrays::array_t<int, 2> src{ { 3, 4 } };
    zx::arrays::array_t<int, 2> dst{ { 4, 5 } };

    const auto [src_bounds, dst_bounds] = zx::arrays::adjust_bounds(dst.bounds(), src.bounds(), { -1, 2 });

    EXPECT_THAT(src_bounds[0], testing::Eq((zx::mat::interval_t<zx::arrays::size_base_t>{ 1, 3 })));
    EXPECT_THAT(src_bounds[1], testing::Eq((zx::mat::interval_t<zx::arrays::size_base_t>{ 0, 3 })));

    EXPECT_THAT(dst_bounds[0], testing::Eq((zx::mat::interval_t<zx::arrays::size_base_t>{ 0, 2 })));
    EXPECT_THAT(dst_bounds[1], testing::Eq((zx::mat::interval_t<zx::arrays::size_base_t>{ 2, 5 })));
}
