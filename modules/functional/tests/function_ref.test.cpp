#include <gmock/gmock.h>

#include <zx/function_ref.hpp>

TEST(function_ref, basic_usage)
{
    zx::function_ref<int(int, int)> add = [](int a, int b) { return a + b; };
    zx::function_ref<int(int, int)> multiply = [](int a, int b) { return a * b; };

    EXPECT_THAT(add(2, 3), 5);
    EXPECT_THAT(multiply(2, 3), 6);
}

TEST(function_ref, lambda_with_captured_value)
{
    const int value = 10;
    zx::function_ref<int(int, int)> add = [c = 100](int a, int b) { return a + b + c; };
    zx::function_ref<int(int, int)> multiply = [&c = value](int a, int b) { return a * b + c; };

    EXPECT_THAT(add(2, 3), 105);
    EXPECT_THAT(multiply(2, 3), 16);
}
