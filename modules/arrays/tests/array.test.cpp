#include <gmock/gmock.h>

#include <zx/array.hpp>

TEST(array, slicing)
{
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ {}, {}, {} })), (zx::arrays::dim_t{ 10, 1, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 3, {}, {} })), (zx::arrays::dim_t{ 7, 1, 3 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ {}, 7, {} })), (zx::arrays::dim_t{ 7, 1, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 2, 8, {} })), (zx::arrays::dim_t{ 6, 1, 2 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 0, 10, 3 })), (zx::arrays::dim_t{ 4, 3, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 2, 8, 2 })), (zx::arrays::dim_t{ 3, 2, 2 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ -8, -2, {} })), (zx::arrays::dim_t{ 6, 1, 2 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ -5, {}, {} })), (zx::arrays::dim_t{ 5, 1, 5 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ {}, -3, {} })), (zx::arrays::dim_t{ 7, 1, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ {}, {}, -1 })), (zx::arrays::dim_t{ 10, -1, 9 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 8, 2, -2 })), (zx::arrays::dim_t{ 3, -2, 8 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 9, {}, -1 })), (zx::arrays::dim_t{ 10, -1, 9 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 0, 100, {} })), (zx::arrays::dim_t{ 10, 1, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ -100, 5, {} })), (zx::arrays::dim_t{ 5, 1, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 0 }.slice({ 5, 5, {} })), (zx::arrays::dim_t{ 0, 1, 5 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 2, 0 }.slice({ 1, 5, {} })), (zx::arrays::dim_t{ 4, 2, 2 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 2, 0 }.slice({ 0, 10, 2 })), (zx::arrays::dim_t{ 5, 4, 0 }));
    EXPECT_THAT((zx::arrays::dim_t{ 10, 1, 5 }.slice({ 2, 8, {} })), (zx::arrays::dim_t{ 6, 1, 7 }));
}

TEST(array, array_1d)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    a[1] = 42;
    EXPECT_THAT(a.shape(), (zx::arrays::shape_t<1>{ { zx::arrays::dim_t{ 10, 1, 0 } } }));
    EXPECT_THAT(a.size(), 10);
    EXPECT_THAT(a.stride(), 1);
    EXPECT_THAT(a.start(), 0);
    EXPECT_THAT(a.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 42);

    auto view = a.view();
    EXPECT_THAT(view.size(), 10);
    EXPECT_THAT(view.stride(), 1);
    EXPECT_THAT(view.start(), 0);
    EXPECT_THAT(view.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(view[0], 0);
    EXPECT_THAT(view[1], 42);

    EXPECT_THAT(view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));

    auto mut_view = a.mut_view();
    EXPECT_THAT(mut_view.size(), 10);
    EXPECT_THAT(mut_view.stride(), 1);
    EXPECT_THAT(mut_view.start(), 0);
    EXPECT_THAT(a.bounds(), (zx::mat::interval_t<int>{ 0, 10 }));
    EXPECT_THAT(mut_view.volume(), 10);
    EXPECT_THAT(mut_view[0], 0);
    EXPECT_THAT(mut_view[1], 42);

    EXPECT_THAT(mut_view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));
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
    EXPECT_THAT(view.start(), 2);
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
    EXPECT_THAT(view.start(), 2);
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
    EXPECT_THAT(view.start(), 2);
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
    EXPECT_THAT(view.start(), 8);
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
    EXPECT_THAT(a[0][0], 1);
    EXPECT_THAT(a[0][1], 42);
    EXPECT_THAT(a[1][0], 53);
    EXPECT_THAT(a.m_data, testing::ElementsAreArray({ 1, 42, 0, 0, 53, 0, 0, 0, -1, -1, -1, -1 }));
}
