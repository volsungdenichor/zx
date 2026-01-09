#include <gmock/gmock.h>

#include <zx/function_ref.hpp>

TEST(function_ref, basic_usage)
{
    const auto add_lambda = [](int a, int b) { return a + b; };
    const auto multiply_lambda = [](int a, int b) { return a * b; };
    zx::function_ref<int(int, int)> add = add_lambda;
    zx::function_ref<int(int, int)> multiply = multiply_lambda;

    EXPECT_THAT(add(2, 3), 5);
    EXPECT_THAT(multiply(2, 3), 6);
}

TEST(function_ref, lambda_with_captured_value)
{
    const int value = 10;
    const auto add_lambda = [c = 100](int a, int b) { return a + b + c; };
    const auto multiply_lambda = [&c = value](int a, int b) { return a * b + c; };
    zx::function_ref<int(int, int)> add = add_lambda;
    zx::function_ref<int(int, int)> multiply = multiply_lambda;

    EXPECT_THAT(add(2, 3), 105);
    EXPECT_THAT(multiply(2, 3), 16);
}

TEST(function_ref, assignment_of_different_lambdas)
{
    const auto add_lambda = [](int a, int b) { return a + b; };
    const auto multiply_lambda = [](int a, int b) { return a * b; };
    zx::function_ref<int(int, int)> add = add_lambda;
    EXPECT_THAT(add(2, 3), 5);
    add = multiply_lambda;
    EXPECT_THAT(add(2, 3), 6);
}
