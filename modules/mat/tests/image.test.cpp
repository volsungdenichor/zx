#include <gmock/gmock.h>

#include <numeric>
#include <sstream>
#include <zx/image.hpp>

TEST(image, load_image)
{
    const auto img = zx::mat::load_bitmap(zx::mat::filepath_t{ std::string(TEST_DATA_DIR) + "/test-24.bmp" });
    EXPECT_THAT(img.extent(), (zx::mat::rgb_image_t::extent_type{ 8, 4 }));
    EXPECT_THAT(img.channel_count(), 3);

    EXPECT_THAT((img[{ 0, 0 }]), (zx::mat::true_color_t{ 255, 0, 0 }));
    EXPECT_THAT((img[{ 1, 0 }]), (zx::mat::true_color_t{ 255, 106, 0 }));
    EXPECT_THAT((img[{ 2, 0 }]), (zx::mat::true_color_t{ 255, 216, 0 }));

    EXPECT_THAT((img[{ 0, 1 }]), (zx::mat::true_color_t{ 127, 0, 0 }));
    EXPECT_THAT((img[{ 1, 1 }]), (zx::mat::true_color_t{ 127, 51, 0 }));
    EXPECT_THAT((img[{ 2, 1 }]), (zx::mat::true_color_t{ 127, 106, 0 }));

    EXPECT_THAT((img[{ 0, 2 }]), (zx::mat::true_color_t{ 255, 127, 127 }));
    EXPECT_THAT((img[{ 1, 2 }]), (zx::mat::true_color_t{ 255, 178, 127 }));
    EXPECT_THAT((img[{ 2, 2 }]), (zx::mat::true_color_t{ 255, 233, 127 }));

    const auto red_channel = img.channel(0);
    EXPECT_THAT((red_channel[{ 0, 0 }]), 255);
    EXPECT_THAT((red_channel[{ 1, 0 }]), 255);
    EXPECT_THAT((red_channel[{ 2, 0 }]), 255);

    EXPECT_THAT((red_channel[{ 0, 1 }]), 127);
    EXPECT_THAT((red_channel[{ 1, 1 }]), 127);
    EXPECT_THAT((red_channel[{ 2, 1 }]), 127);

    EXPECT_THAT((red_channel[{ 0, 2 }]), 255);
    EXPECT_THAT((red_channel[{ 1, 2 }]), 255);
    EXPECT_THAT((red_channel[{ 2, 2 }]), 255);

    const auto green_channel = img.channel(1);
    EXPECT_THAT((green_channel[{ 0, 0 }]), 0);
    EXPECT_THAT((green_channel[{ 1, 0 }]), 106);
    EXPECT_THAT((green_channel[{ 2, 0 }]), 216);

    EXPECT_THAT((green_channel[{ 0, 1 }]), 0);
    EXPECT_THAT((green_channel[{ 1, 1 }]), 51);
    EXPECT_THAT((green_channel[{ 2, 1 }]), 106);

    EXPECT_THAT((green_channel[{ 0, 2 }]), 127);
    EXPECT_THAT((green_channel[{ 1, 2 }]), 178);
    EXPECT_THAT((green_channel[{ 2, 2 }]), 233);

    const auto blue_channel = img.channel(2);
    EXPECT_THAT((blue_channel[{ 0, 0 }]), 0);
    EXPECT_THAT((blue_channel[{ 1, 0 }]), 0);
    EXPECT_THAT((blue_channel[{ 2, 0 }]), 0);

    EXPECT_THAT((blue_channel[{ 0, 1 }]), 0);
    EXPECT_THAT((blue_channel[{ 1, 1 }]), 0);
    EXPECT_THAT((blue_channel[{ 2, 1 }]), 0);

    EXPECT_THAT((blue_channel[{ 0, 2 }]), 127);
    EXPECT_THAT((blue_channel[{ 1, 2 }]), 127);
    EXPECT_THAT((blue_channel[{ 2, 2 }]), 127);
}

namespace
{

using zx::mat::rgb_image_t;
using zx::mat::true_color_t;

rgb_image_t make_numbered_image(zx::mat::extent_base_t w, zx::mat::extent_base_t h)
{
    rgb_image_t img{ rgb_image_t::extent_type{ w, h } };
    for (zx::mat::location_base_t y = 0; y < h; ++y)
    {
        for (zx::mat::location_base_t x = 0; x < w; ++x)
        {
            img[{ x, y }] = true_color_t{ static_cast<zx::mat::byte_t>(x), static_cast<zx::mat::byte_t>(y), 0 };
        }
    }
    return img;
}

std::vector<true_color_t> pixels(const rgb_image_t::view_type& img)
{
    return std::vector<true_color_t>(img.begin(), img.end());
}

true_color_t xy(int x, int y)
{
    return true_color_t{ static_cast<zx::mat::byte_t>(x), static_cast<zx::mat::byte_t>(y), 0 };
}

}  // namespace

TEST(image, extent_is_width_height)
{
    const rgb_image_t img{ rgb_image_t::extent_type{ 3, 2 } };
    EXPECT_THAT(img.extent(), (rgb_image_t::extent_type{ 3, 2 }));
    EXPECT_THAT(img.volume(), 6);
    EXPECT_NO_THROW((img[{ 2, 1 }]));
    EXPECT_THROW((img[{ 1, 2 }]), std::out_of_range);
}

TEST(image, channels_are_interleaved_and_rows_are_contiguous)
{
    const rgb_image_t img{ rgb_image_t::extent_type{ 4, 2 } };
    EXPECT_THAT(img.data().stride(), (rgb_image_t::storage_type::stride_type{ 1, 3, 12 }));
}

TEST(image, iteration_is_lateral_first)
{
    const auto img = make_numbered_image(3, 2);
    EXPECT_THAT(pixels(img), testing::ElementsAre(xy(0, 0), xy(1, 0), xy(2, 0), xy(0, 1), xy(1, 1), xy(2, 1)));
}

TEST(image, mutable_iteration)
{
    auto img = make_numbered_image(3, 2);
    std::fill(img.begin(), img.end(), true_color_t{ 1, 2, 3 });
    EXPECT_THAT(pixels(img), testing::Each(true_color_t{ 1, 2, 3 }));

    const auto src = make_numbered_image(3, 2);
    std::copy(src.begin(), src.end(), img.begin());
    EXPECT_THAT(pixels(img), testing::ElementsAreArray(pixels(src)));
}

TEST(image, for_each_is_lateral_first)
{
    std::vector<rgb_image_t::location_type> locs;
    zx::mat::detail::for_each(
        rgb_image_t::extent_type{ 2, 2 }, [&](const rgb_image_t::location_type& loc) { locs.push_back(loc); });
    EXPECT_THAT(
        locs,
        testing::ElementsAre(
            rgb_image_t::location_type{ 0, 0 },
            rgb_image_t::location_type{ 1, 0 },
            rgb_image_t::location_type{ 0, 1 },
            rgb_image_t::location_type{ 1, 1 }));
}

TEST(image, slice_uses_x_y)
{
    const auto img = make_numbered_image(4, 3);
    const auto s = img.slice({ { 1, 3 }, { 2, 3 } });
    EXPECT_THAT(s.extent(), (rgb_image_t::extent_type{ 2, 1 }));
    EXPECT_THAT(pixels(s), testing::ElementsAre(xy(1, 2), xy(2, 2)));
}

TEST(image, flip)
{
    const auto img = make_numbered_image(3, 2);
    EXPECT_THAT(
        pixels(zx::mat::flip_horizontal(img.view())),
        testing::ElementsAre(xy(2, 0), xy(1, 0), xy(0, 0), xy(2, 1), xy(1, 1), xy(0, 1)));
    EXPECT_THAT(
        pixels(zx::mat::flip_vertical(img.view())),
        testing::ElementsAre(xy(0, 1), xy(1, 1), xy(2, 1), xy(0, 0), xy(1, 0), xy(2, 0)));
}

TEST(image, rotate_clockwise)
{
    const auto img = make_numbered_image(3, 2);

    const auto r90 = zx::mat::rotate(img.view(), 90);
    EXPECT_THAT(r90.extent(), (rgb_image_t::extent_type{ 2, 3 }));
    EXPECT_THAT(pixels(r90), testing::ElementsAre(xy(0, 1), xy(0, 0), xy(1, 1), xy(1, 0), xy(2, 1), xy(2, 0)));

    const auto r180 = zx::mat::rotate(img.view(), 180);
    EXPECT_THAT(pixels(r180), testing::ElementsAre(xy(2, 1), xy(1, 1), xy(0, 1), xy(2, 0), xy(1, 0), xy(0, 0)));

    const auto r270 = zx::mat::rotate(img.view(), -90);
    EXPECT_THAT(r270.extent(), (rgb_image_t::extent_type{ 2, 3 }));
    EXPECT_THAT(pixels(r270), testing::ElementsAre(xy(2, 0), xy(2, 1), xy(1, 0), xy(1, 1), xy(0, 0), xy(0, 1)));
}

TEST(image, rotate_matrix_array_unchanged)
{
    zx::mat::array_t<int, 2> m{ { 2, 3 } };
    std::iota(m.begin(), m.end(), 0);
    const auto r = zx::mat::rotate(m.view(), 90);
    EXPECT_THAT(r.extent(), (zx::mat::array_t<int, 2>::extent_type{ 3, 2 }));
    EXPECT_THAT(std::vector<int>(r.begin(), r.end()), testing::ElementsAre(3, 0, 4, 1, 5, 2));
}

TEST(image, paste_at_x_y)
{
    rgb_image_t dst{ rgb_image_t::extent_type{ 4, 3 } };
    rgb_image_t src{ rgb_image_t::extent_type{ 2, 1 } };
    std::fill(src.begin(), src.end(), true_color_t{ 9, 9, 9 });
    zx::mat::paste(dst.mut_view(), src.view(), rgb_image_t::location_type{ 1, 2 });
    EXPECT_THAT((dst[{ 1, 2 }]), (true_color_t{ 9, 9, 9 }));
    EXPECT_THAT((dst[{ 2, 2 }]), (true_color_t{ 9, 9, 9 }));
    EXPECT_THAT((dst[{ 0, 2 }]), (true_color_t{ 0, 0, 0 }));
    EXPECT_THAT((dst[{ 2, 1 }]), (true_color_t{ 0, 0, 0 }));
}

TEST(image, draw_rasterized_rectangle)
{
    rgb_image_t img{ rgb_image_t::extent_type{ 5, 4 } };
    const auto rect = zx::mat::box::from_lower_upper(zx::mat::point(1, 2), zx::mat::point(4, 3));
    zx::mat::draw(img.mut_view(), zx::mat::rasterize(rect), zx::mat::filters::solid(true_color_t{ 255, 0, 0 }));
    for (zx::mat::location_base_t y = 0; y < 4; ++y)
    {
        for (zx::mat::location_base_t x = 0; x < 5; ++x)
        {
            const bool inside = x >= 1 && x < 4 && y == 2;
            EXPECT_THAT((img[{ x, y }]), (inside ? true_color_t{ 255, 0, 0 } : true_color_t{ 0, 0, 0 })) << x << " " << y;
        }
    }
}

TEST(image, bitmap_round_trip)
{
    const auto img = make_numbered_image(5, 3);
    std::stringstream ss;
    zx::mat::save_bitmap(img.view(), ss);
    const auto loaded = zx::mat::load_bitmap(ss);
    EXPECT_THAT(loaded.extent(), (rgb_image_t::extent_type{ 5, 3 }));
    EXPECT_THAT(pixels(loaded), testing::ElementsAreArray(pixels(img)));
}
