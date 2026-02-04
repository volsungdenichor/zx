#include <gmock/gmock.h>

#include <zx/let.hpp>

TEST(let, no_args)
{
    auto result = zx::let([]() { return 42; });
    EXPECT_THAT(result, 42);
}

TEST(let, one_arg)
{
    auto result = zx::let(10, [](int a) { return a * 2; });
    EXPECT_THAT(result, 20);
}

TEST(let, two_args)
{
    auto result = zx::let(10, 32, [](int a, int b) { return a + b; });
    EXPECT_THAT(result, 42);
}

TEST(let, three_args)
{
    auto result = zx::let(10, 20, 12, [](int a, int b, int c) { return a + b + c; });
    EXPECT_THAT(result, 42);
}

TEST(let, four_args)
{
    auto result = zx::let(10, 10, 10, 12, [](int a, int b, int c, int d) { return a + b + c + d; });
    EXPECT_THAT(result, 42);
}

TEST(let, five_args)
{
    auto result = zx::let(10, 10, 10, 10, 2, [](int a, int b, int c, int d, int e) { return a + b + c + d + e; });
    EXPECT_THAT(result, 42);
}

TEST(let, six_args)
{
    auto result = zx::let(7, 7, 7, 7, 7, 7, [](int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; });
    EXPECT_THAT(result, 42);
}

TEST(let, forwarding)
{
    struct move_only
    {
        move_only(int v) : value(v) { }
        move_only(const move_only&) = delete;
        move_only(move_only&& other) noexcept : value(other.value) { other.value = 0; }
        int value;
    };

    auto result = zx::let(move_only(42), [](move_only&& mo) { return mo.value; });
    EXPECT_THAT(result, 42);
}

TEST(let, const_ref)
{
    int x = 42;
    auto result = zx::let(x, [](const int& v) { return v; });
    EXPECT_THAT(result, 42);
}

TEST(let, lvalue_ref)
{
    int x = 42;
    auto result = zx::let(
        x,
        [](int& v)
        {
            v = 100;
            return v;
        });
    EXPECT_THAT(result, 100);
    EXPECT_THAT(x, 100);
}

TEST(let, mixed_args)
{
    int x = 10;
    auto result = zx::let(
        x,
        32,
        [](int& a, int b)
        {
            a += 10;
            return a + b;
        });
    EXPECT_THAT(result, 52);
    EXPECT_THAT(x, 20);
}

TEST(let, capture_by_value)
{
    int x = 10;
    auto result = zx::let(
        x,
        [](int a)
        {
            a += 32;
            return a;
        });
    EXPECT_THAT(result, 42);
    EXPECT_THAT(x, 10);
}

TEST(let, capture_by_reference)
{
    int x = 10;
    auto result = zx::let(
        x,
        [](int& a)
        {
            a += 32;
            return a;
        });
    EXPECT_THAT(result, 42);
    EXPECT_THAT(x, 42);
}

TEST(let, multiple_invocations)
{
    int x = 5;
    auto func = [](int& a) { return a *= 2; };

    auto result1 = zx::let(x, func);
    EXPECT_THAT(result1, 10);
    EXPECT_THAT(x, 10);

    auto result2 = zx::let(x, func);
    EXPECT_THAT(result2, 20);
    EXPECT_THAT(x, 20);

    auto result3 = zx::let(x, func);
    EXPECT_THAT(result3, 40);
    EXPECT_THAT(x, 40);
}

TEST(let, nested_let)
{
    int x = 10;
    auto result = zx::let(
        x,
        [](int& a)
        {
            return zx::let(
                a,
                [](int& b)
                {
                    b += 22;
                    return b;
                });
        });
    EXPECT_THAT(result, 32);
    EXPECT_THAT(x, 32);
}

TEST(let, let_with_lambda_returning_void)
{
    int x = 10;
    zx::let(x, [](int& a) { a += 32; });
    EXPECT_THAT(x, 42);
}

TEST(let, let_no_args_lambda_returning_void)
{
    int x = 0;
    zx::let([&]() { x = 42; });
    EXPECT_THAT(x, 42);
}

TEST(let, let_with_move_only_lambda)
{
    auto move_only_lambda = [ptr = std::make_unique<int>(42)]() { return *ptr; };
    auto result = zx::let(move_only_lambda);
    EXPECT_THAT(result, 42);
}

TEST(let, let_with_no_args_and_no_return)
{
    int x = 0;
    zx::let([&]() { x = 42; });
    EXPECT_THAT(x, 42);
}

TEST(let, let_with_multiple_move_only_args)
{
    auto mo1 = std::make_unique<int>(10);
    auto mo2 = std::make_unique<int>(32);

    auto result
        = zx::let(std::move(mo1), std::move(mo2), [](std::unique_ptr<int> a, std::unique_ptr<int> b) { return *a + *b; });

    EXPECT_THAT(result, 42);
}

TEST(let, let_with_const_move_only_arg)
{
    const auto mo = std::make_unique<int>(42);

    auto result = zx::let(mo, [](const std::unique_ptr<int>& a) { return *a; });

    EXPECT_THAT(result, 42);
}

TEST(let, let_with_mixed_move_only_and_lvalue_refs)
{
    int x = 10;
    auto mo = std::make_unique<int>(32);

    auto result = zx::let(
        x,
        std::move(mo),
        [](int& a, std::unique_ptr<int> b)
        {
            a += 10;
            return a + *b;
        });

    EXPECT_THAT(result, 52);
    EXPECT_THAT(x, 20);
}

TEST(let, let_with_no_args_lambda_returning_move_only)
{
    auto result = zx::let([]() { return std::make_unique<int>(42); });
    EXPECT_THAT(*result, 42);
}
