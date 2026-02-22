#include <gmock/gmock.h>

#include <zx/array.hpp>

TEST(array, array_1d)
{
    zx::arrays::array_t<int, 1> a{ 10 };
    a[1] = 42;
    EXPECT_THAT(a.shape(), (zx::arrays::shape_t<1>{ { zx::arrays::dim_t{ 10, 1, 0 } } }));
    EXPECT_THAT(a.size(), testing::ElementsAre(10));
    EXPECT_THAT(a.stride(), testing::ElementsAre(1));
    EXPECT_THAT(a.start(), testing::ElementsAre(0));
    EXPECT_THAT(a.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 10 } }));
    EXPECT_THAT(a[0], 0);
    EXPECT_THAT(a[1], 42);

    auto view = a.view();
    EXPECT_THAT(view.size(), testing::ElementsAre(10));
    EXPECT_THAT(view.stride(), testing::ElementsAre(1));
    EXPECT_THAT(view.start(), testing::ElementsAre(0));
    EXPECT_THAT(view.volume(), 10);
    EXPECT_THAT(a.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 10 } }));
    EXPECT_THAT(view[0], 0);
    EXPECT_THAT(view[1], 42);

    EXPECT_THAT(view, testing::ElementsAreArray({ 0, 42, 0, 0, 0, 0, 0, 0, 0, 0 }));

    auto mut_view = a.mut_view();
    EXPECT_THAT(mut_view.size(), testing::ElementsAre(10));
    EXPECT_THAT(mut_view.stride(), testing::ElementsAre(1));
    EXPECT_THAT(mut_view.start(), testing::ElementsAre(0));
    EXPECT_THAT(a.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 10 } }));
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
    auto view = a.view().slice({ { 2, 8 } });
    EXPECT_THAT(view.size(), testing::ElementsAre(6));
    EXPECT_THAT(view.stride(), testing::ElementsAre(1));
    EXPECT_THAT(view.start(), testing::ElementsAre(2));
    EXPECT_THAT(view.volume(), 6);
    EXPECT_THAT(view.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 6 } }));
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
    auto view = a.view().slice({ { 2, 8, 2 } });
    EXPECT_THAT(view.size(), testing::ElementsAre(3));
    EXPECT_THAT(view.stride(), testing::ElementsAre(2));
    EXPECT_THAT(view.start(), testing::ElementsAre(2));
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 3 } }));
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
    auto view = a.view().slice({ { -8, -2 } });
    EXPECT_THAT(view.size(), testing::ElementsAre(6));
    EXPECT_THAT(view.stride(), testing::ElementsAre(1));
    EXPECT_THAT(view.start(), testing::ElementsAre(2));
    EXPECT_THAT(view.volume(), 6);
    EXPECT_THAT(view.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 6 } }));
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
    for (int i = 0; i < 10; ++i)
    {
        a[i] = i;
    }
    auto view = a.view().slice({ { 8, 2, -2 } });
    EXPECT_THAT(view.size(), testing::ElementsAre(3));
    EXPECT_THAT(view.stride(), testing::ElementsAre(-2));
    EXPECT_THAT(view.start(), testing::ElementsAre(8));
    EXPECT_THAT(view.volume(), 3);
    EXPECT_THAT(view.bounds(), (zx::mat::box_shape<int, 1>{ { 0, 3 } }));
    EXPECT_THAT(view[0], 8);
    EXPECT_THAT(view[1], 6);
    EXPECT_THAT(view[2], 4);
    EXPECT_THAT(view, testing::ElementsAreArray({ 8, 6, 4 }));
}