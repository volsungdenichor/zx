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
}