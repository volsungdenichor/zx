#include <gmock/gmock.h>

#include <zx/image.hpp>

TEST(image, load_image)
{
    const auto img = zx::images::load_bitmap(std::string(TEST_DATA_DIR) + "/test-24.bmp");
    EXPECT_THAT(img.size(), (zx::images::rgb_image_t::size_type{ 4, 8, 3 }));

    EXPECT_THAT((img[{ 0, 0, 0 }]), 255);
    EXPECT_THAT((img[{ 0, 0, 1 }]), 0);
    EXPECT_THAT((img[{ 0, 0, 2 }]), 0);

    EXPECT_THAT((img[{ 0, 1, 0 }]), 255);
    EXPECT_THAT((img[{ 0, 1, 1 }]), 106);
    EXPECT_THAT((img[{ 0, 1, 2 }]), 0);

    EXPECT_THAT((img[{ 0, 2, 0 }]), 255);
    EXPECT_THAT((img[{ 0, 2, 1 }]), 216);
    EXPECT_THAT((img[{ 0, 2, 2 }]), 0);

    EXPECT_THAT((img[{ 1, 0, 0 }]), 127);
    EXPECT_THAT((img[{ 1, 0, 1 }]), 0);
    EXPECT_THAT((img[{ 1, 0, 2 }]), 0);

    EXPECT_THAT((img[{ 1, 1, 0 }]), 127);
    EXPECT_THAT((img[{ 1, 1, 1 }]), 51);
    EXPECT_THAT((img[{ 1, 1, 2 }]), 0);

    EXPECT_THAT((img[{ 1, 2, 0 }]), 127);
    EXPECT_THAT((img[{ 1, 2, 1 }]), 106);
    EXPECT_THAT((img[{ 1, 2, 2 }]), 0);

    EXPECT_THAT((img[{ 2, 0, 0 }]), 255);
    EXPECT_THAT((img[{ 2, 0, 1 }]), 127);
    EXPECT_THAT((img[{ 2, 0, 2 }]), 127);

    EXPECT_THAT((img[{ 2, 1, 0 }]), 255);
    EXPECT_THAT((img[{ 2, 1, 1 }]), 178);
    EXPECT_THAT((img[{ 2, 1, 2 }]), 127);

    EXPECT_THAT((img[{ 2, 2, 0 }]), 255);
    EXPECT_THAT((img[{ 2, 2, 1 }]), 233);
    EXPECT_THAT((img[{ 2, 2, 2 }]), 127);

    EXPECT_THAT(img[-1][-1], testing::ElementsAre(96, 96, 96));

    EXPECT_THAT(zx::images::at(img, { 0, 0 }), (zx::images::rgb_color_t{ 255, 0, 0 }));
    EXPECT_THAT(zx::images::at(img, { 0, 1 }), (zx::images::rgb_color_t{ 255, 106, 0 }));
    EXPECT_THAT(zx::images::at(img, { 0, 2 }), (zx::images::rgb_color_t{ 255, 216, 0 }));

    EXPECT_THAT(zx::images::at(img, { 1, 0 }), (zx::images::rgb_color_t{ 127, 0, 0 }));
    EXPECT_THAT(zx::images::at(img, { 1, 1 }), (zx::images::rgb_color_t{ 127, 51, 0 }));
    EXPECT_THAT(zx::images::at(img, { 1, 2 }), (zx::images::rgb_color_t{ 127, 106, 0 }));

    EXPECT_THAT(zx::images::at(img, { 2, 0 }), (zx::images::rgb_color_t{ 255, 127, 127 }));
    EXPECT_THAT(zx::images::at(img, { 2, 1 }), (zx::images::rgb_color_t{ 255, 178, 127 }));
    EXPECT_THAT(zx::images::at(img, { 2, 2 }), (zx::images::rgb_color_t{ 255, 233, 127 }));

    const auto red_channel = zx::images::channel(img, 0);
    EXPECT_THAT((red_channel[{ 0, 0 }]), 255);
    EXPECT_THAT((red_channel[{ 0, 1 }]), 255);
    EXPECT_THAT((red_channel[{ 0, 2 }]), 255);

    EXPECT_THAT((red_channel[{ 1, 0 }]), 127);
    EXPECT_THAT((red_channel[{ 1, 1 }]), 127);
    EXPECT_THAT((red_channel[{ 1, 2 }]), 127);

    EXPECT_THAT((red_channel[{ 2, 0 }]), 255);
    EXPECT_THAT((red_channel[{ 2, 1 }]), 255);
    EXPECT_THAT((red_channel[{ 2, 2 }]), 255);

    const auto green_channel = zx::images::channel(img, 1);
    EXPECT_THAT((green_channel[{ 0, 0 }]), 0);
    EXPECT_THAT((green_channel[{ 0, 1 }]), 106);
    EXPECT_THAT((green_channel[{ 0, 2 }]), 216);

    EXPECT_THAT((green_channel[{ 1, 0 }]), 0);
    EXPECT_THAT((green_channel[{ 1, 1 }]), 51);
    EXPECT_THAT((green_channel[{ 1, 2 }]), 106);

    EXPECT_THAT((green_channel[{ 2, 0 }]), 127);
    EXPECT_THAT((green_channel[{ 2, 1 }]), 178);
    EXPECT_THAT((green_channel[{ 2, 2 }]), 233);

    const auto blue_channel = zx::images::channel(img, 2);
    EXPECT_THAT((blue_channel[{ 0, 0 }]), 0);
    EXPECT_THAT((blue_channel[{ 0, 1 }]), 0);
    EXPECT_THAT((blue_channel[{ 0, 2 }]), 0);

    EXPECT_THAT((blue_channel[{ 1, 0 }]), 0);
    EXPECT_THAT((blue_channel[{ 1, 1 }]), 0);
    EXPECT_THAT((blue_channel[{ 1, 2 }]), 0);

    EXPECT_THAT((blue_channel[{ 2, 0 }]), 127);
    EXPECT_THAT((blue_channel[{ 2, 1 }]), 127);
    EXPECT_THAT((blue_channel[{ 2, 2 }]), 127);
}